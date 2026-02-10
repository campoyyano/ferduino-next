#include "app/scheduler/app_event_scheduler.h"

#include <string.h>

#include "app/scheduler/app_scheduler.h"
#include "app/nvm/eeprom_registry.h"

namespace app::scheduler::events {

static Window g_windows[kMaxChannels];
static uint8_t g_desired[kMaxChannels]; // 0/1
static uint8_t g_changed[kMaxChannels]; // 0/1

// Registry keys (TLV)
// 330: scheduler.events.valid (bool)
// 331..339: scheduler.events.chN.enabled (bool)
// 340..348: scheduler.events.chN.onMinute (u32)
// 350..358: scheduler.events.chN.offMinute (u32)
static constexpr uint16_t kKeyValid        = 330;
static constexpr uint16_t kKeyEnabledBase  = 331; // +ch
static constexpr uint16_t kKeyOnBase       = 340; // +ch
static constexpr uint16_t kKeyOffBase      = 350; // +ch

static uint16_t clampMinute(uint16_t m) {
  return (m >= 1440) ? (uint16_t)(m % 1440) : m;
}

static uint16_t hmToMinute(app::scheduler::TimeHM t) {
  const uint16_t h = (t.hour > 23) ? 23 : t.hour;
  const uint16_t m = (t.minute > 59) ? 59 : t.minute;
  return (uint16_t)(h * 60u + m);
}

static app::scheduler::TimeHM minuteToHM(uint16_t minuteOfDay) {
  app::scheduler::TimeHM t{};
  const uint16_t m = clampMinute(minuteOfDay);
  t.hour = (uint8_t)(m / 60u);
  t.minute = (uint8_t)(m % 60u);
  return t;
}

bool computeDesired(const Window& w, uint16_t minuteOfDay) {
  if (!w.enabled) return false;

  const uint16_t on = clampMinute(w.onMinute);
  const uint16_t off = clampMinute(w.offMinute);
  const uint16_t now = clampMinute(minuteOfDay);

  if (on == off) {
    // Ventana degenerada: interpretamos como "siempre OFF"
    return false;
  }

  if (on < off) {
    // Ventana normal: [on, off)
    return (now >= on) && (now < off);
  }

  // Ventana cruza medianoche: ON si now >= on OR now < off
  return (now >= on) || (now < off);
}

static void applyDesiredInitial() {
  const uint16_t nowMin = app::scheduler::minuteOfDay();
  for (uint8_t ch = 0; ch < kMaxChannels; ++ch) {
    const bool want = computeDesired(g_windows[ch], nowMin);
    g_desired[ch] = want ? 1u : 0u;
    g_changed[ch] = 1u; // forzar "primera aplicación" (útil cuando C2 conecte a outlets)
  }
}

static void loadFromRegistry() {
  auto& reg = app::nvm::registry();

  bool valid = false;
  if (!reg.getBool(kKeyValid, valid) || !valid) {
    return;
  }

  for (uint8_t ch = 0; ch < kMaxChannels; ++ch) {
    bool en = false;
    uint32_t on = 0;
    uint32_t off = 0;

    (void)reg.getBool((uint16_t)(kKeyEnabledBase + ch), en);
    (void)reg.getU32((uint16_t)(kKeyOnBase + ch), on);
    (void)reg.getU32((uint16_t)(kKeyOffBase + ch), off);

    g_windows[ch].enabled = en;
    g_windows[ch].onMinute = clampMinute((uint16_t)on);
    g_windows[ch].offMinute = clampMinute((uint16_t)off);
  }
}

static void persistChannel(uint8_t ch) {
  auto& reg = app::nvm::registry();

  (void)reg.beginEdit();
  (void)reg.setBool(kKeyValid, true);
  (void)reg.setBool((uint16_t)(kKeyEnabledBase + ch), g_windows[ch].enabled);
  (void)reg.setU32((uint16_t)(kKeyOnBase + ch), (uint32_t)clampMinute(g_windows[ch].onMinute));
  (void)reg.setU32((uint16_t)(kKeyOffBase + ch), (uint32_t)clampMinute(g_windows[ch].offMinute));
  (void)reg.endEdit();
}

void begin() {
  memset(g_windows, 0, sizeof(g_windows));
  memset(g_desired, 0, sizeof(g_desired));
  memset(g_changed, 0, sizeof(g_changed));

  // Defaults seguros: todo disabled.
  for (uint8_t ch = 0; ch < kMaxChannels; ++ch) {
    g_windows[ch].onMinute = 0;
    g_windows[ch].offMinute = 0;
    g_windows[ch].enabled = false;
  }

  // C1.2: cargar desde TLV si existe
  loadFromRegistry();

  // Inicializar estado deseado coherente con la hora actual
  applyDesiredInitial();
}

void loop() {
  // Sólo re-evaluar cuando cambia el minuto.
  if (!app::scheduler::consumeMinuteTick()) return;

  const uint16_t nowMin = app::scheduler::minuteOfDay();

  for (uint8_t ch = 0; ch < kMaxChannels; ++ch) {
    const bool want = computeDesired(g_windows[ch], nowMin);
    const uint8_t wantU8 = want ? 1u : 0u;
    if (wantU8 != g_desired[ch]) {
      g_desired[ch] = wantU8;
      g_changed[ch] = 1u;
    }
  }
}

bool setWindow(uint8_t channel, app::scheduler::TimeHM on, app::scheduler::TimeHM off, bool enabled) {
  if (channel >= kMaxChannels) return false;

  Window w{};
  w.onMinute = hmToMinute(on);
  w.offMinute = hmToMinute(off);
  w.enabled = enabled;

  g_windows[channel] = w;

  // Persistir el canal (C1.2)
  persistChannel(channel);

  // Re-evaluación inmediata (sin depender del tick) para que el usuario vea el efecto.
  const uint16_t nowMin = app::scheduler::minuteOfDay();
  const bool want = computeDesired(w, nowMin);
  const uint8_t wantU8 = want ? 1u : 0u;
  if (wantU8 != g_desired[channel]) {
    g_desired[channel] = wantU8;
    g_changed[channel] = 1u;
  } else {
    // aunque no cambie desired, marcamos changed para permitir "apply" al actualizar config
    g_changed[channel] = 1u;
  }

  return true;
}

Window window(uint8_t channel) {
  if (channel >= kMaxChannels) {
    Window w{};
    w.onMinute = 0;
    w.offMinute = 0;
    w.enabled = false;
    return w;
  }
  return g_windows[channel];
}

bool desiredOn(uint8_t channel) {
  if (channel >= kMaxChannels) return false;
  return g_desired[channel] != 0;
}

bool consumeDesiredChanged(uint8_t channel) {
  if (channel >= kMaxChannels) return false;
  const bool v = (g_changed[channel] != 0);
  g_changed[channel] = 0;
  return v;
}

} // namespace app::scheduler::events
