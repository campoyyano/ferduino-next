#include "app/outlets/app_outlets.h"

#include "app/nvm/eeprom_registry.h"
#include "app/app_build_flags.h"

#if APP_OUTLETS_USE_RELAY_HAL
#include "hal/hal_relay.h"
#endif

namespace app::outlets {

// Keys TLV (según migración legacy->registry):
// 310 → bloque outlets válido (bool)
// 311..319 → estado outlets (U32 0/1)
static constexpr uint16_t kOutletValidKey     = 310;
static constexpr uint16_t kOutletStateBaseKey = 311;

// C2: modo auto por outlet (arbitraje con scheduler)
// 320 → bloque modo outlets válido (bool)
// 321..329 → outlet.N.auto (bool)
static constexpr uint16_t kOutletModeValidKey = 320;
static constexpr uint16_t kOutletAutoBaseKey  = 321;
static constexpr uint8_t  kOutletCount        = 9;

static uint8_t g_state[kOutletCount] = {0};
static uint8_t g_auto[kOutletCount]  = {0};

static void loadFromRegistry() {
  auto& reg = app::nvm::registry();

  bool valid = false;
  (void)reg.getBool(kOutletValidKey, valid);
  if (!valid) {
    for (uint8_t i = 0; i < kOutletCount; ++i) g_state[i] = 0;
    return;
  }

  for (uint8_t i = 0; i < kOutletCount; ++i) {
    uint32_t v = 0;
    if (reg.getU32((uint16_t)(kOutletStateBaseKey + i), v)) {
      g_state[i] = (v != 0) ? 1 : 0;
    } else {
      g_state[i] = 0;
    }
  }

  // C2: modo auto (opcional)
  bool modeValid = false;
  (void)reg.getBool(kOutletModeValidKey, modeValid);
  if (!modeValid) {
    for (uint8_t i = 0; i < kOutletCount; ++i) g_auto[i] = 0;
    return;
  }

  for (uint8_t i = 0; i < kOutletCount; ++i) {
    bool a = false;
    if (reg.getBool((uint16_t)(kOutletAutoBaseKey + i), a)) {
      g_auto[i] = a ? 1 : 0;
    } else {
      g_auto[i] = 0;
    }
  }
}

static void saveToRegistry() {
  auto& reg = app::nvm::registry();

  (void)reg.setBool(kOutletValidKey, true);

  // C2: persistimos también bloque de modo, para que quede estable.
  (void)reg.setBool(kOutletModeValidKey, true);

  for (uint8_t i = 0; i < kOutletCount; ++i) {
    (void)reg.setU32((uint16_t)(kOutletStateBaseKey + i), g_state[i] ? 1u : 0u);
    (void)reg.setBool((uint16_t)(kOutletAutoBaseKey + i), g_auto[i] ? true : false);
  }

  (void)reg.commit();
}

void begin() {
  loadFromRegistry();

#if APP_OUTLETS_USE_RELAY_HAL
  // Aplicar estado inicial a HAL relés (index 0..8)
  for (uint8_t i = 0; i < kOutletCount; ++i) {
    hal::relay().set(i, g_state[i] ? hal::RelayState::On : hal::RelayState::Off);
  }
#endif
}

static void applyToHal(uint8_t index) {
#if APP_OUTLETS_USE_RELAY_HAL
  hal::relay().set(index, g_state[index] ? hal::RelayState::On : hal::RelayState::Off);
#else
  (void)index;
#endif
}

static void setStateInternal(uint8_t index, bool on) {
  g_state[index] = on ? 1 : 0;
  applyToHal(index);
}

void set(uint8_t index, bool on) {
  if (index >= kOutletCount) return;

  // Manual override: al actuar manualmente, deshabilitamos auto
  g_auto[index] = 0;
  setStateInternal(index, on);

  saveToRegistry();
}

bool get(uint8_t index) {
  if (index >= kOutletCount) return false;
  return g_state[index] != 0;
}

void setAuto(uint8_t index, bool enabled) {
  if (index >= kOutletCount) return;
  g_auto[index] = enabled ? 1 : 0;
  saveToRegistry();
}

bool isAuto(uint8_t index) {
  if (index >= kOutletCount) return false;
  return g_auto[index] != 0;
}

bool applyDesiredIfAuto(uint8_t index, bool desiredOn) {
  if (index >= kOutletCount) return false;
  if (g_auto[index] == 0) return false;

  const uint8_t desired = desiredOn ? 1 : 0;
  if (g_state[index] == desired) {
    return false;
  }

  setStateInternal(index, desiredOn);
  saveToRegistry();
  return true;
}

uint8_t count() {
  return kOutletCount;
}

} // namespace app::outlets
