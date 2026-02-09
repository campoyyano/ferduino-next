#include "app/scheduler/app_event_scheduler.h"

#include <string.h>

#include "app/scheduler/app_scheduler.h"

namespace app::scheduler::events {

static Window g_windows[kMaxChannels];
static uint8_t g_desired[kMaxChannels]; // 0/1
static uint8_t g_changed[kMaxChannels]; // 0/1

static uint16_t hmToMinute(app::scheduler::TimeHM t) {
  const uint16_t h = (t.hour > 23) ? 23 : t.hour;
  const uint16_t m = (t.minute > 59) ? 59 : t.minute;
  return (uint16_t)(h * 60u + m);
}

bool computeDesired(const Window& w, uint16_t minuteOfDay) {
  if (!w.enabled) return false;

  const uint16_t on  = (w.onMinute  >= 1440) ? (uint16_t)(w.onMinute  % 1440) : w.onMinute;
  const uint16_t off = (w.offMinute >= 1440) ? (uint16_t)(w.offMinute % 1440) : w.offMinute;
  const uint16_t now = (minuteOfDay >= 1440) ? (uint16_t)(minuteOfDay % 1440) : minuteOfDay;

  if (on == off) {
    // Ventana degenerada: interpretamos como "siempre OFF"
    return false;
  }

  if (on < off) {
    // Ventana normal: [on, off)
    return (now >= on) && (now < off);
  }

  // Cruza medianoche: ON si now >= on OR now < off
  return (now >= on) || (now < off);
}

void begin() {
  memset(g_windows, 0, sizeof(g_windows));
  memset(g_desired, 0, sizeof(g_desired));
  memset(g_changed, 0, sizeof(g_changed));

  for (uint8_t ch = 0; ch < kMaxChannels; ++ch) {
    g_windows[ch].onMinute = 0;
    g_windows[ch].offMinute = 0;
    g_windows[ch].enabled = false;
  }
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

  // Re-evaluación inmediata para reflejar cambio en desired.
  const uint16_t nowMin = app::scheduler::minuteOfDay();
  const bool want = computeDesired(w, nowMin);
  const uint8_t wantU8 = want ? 1u : 0u;
  if (wantU8 != g_desired[channel]) {
    g_desired[channel] = wantU8;
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
