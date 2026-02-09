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
static constexpr uint8_t  kOutletCount        = 9;

static uint8_t g_state[kOutletCount] = {0};

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
}

static void saveToRegistry() {
  auto& reg = app::nvm::registry();

  (void)reg.setBool(kOutletValidKey, true);

  for (uint8_t i = 0; i < kOutletCount; ++i) {
    (void)reg.setU32((uint16_t)(kOutletStateBaseKey + i), g_state[i] ? 1u : 0u);
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

void set(uint8_t index, bool on) {
  if (index >= kOutletCount) return;

  g_state[index] = on ? 1 : 0;

#if APP_OUTLETS_USE_RELAY_HAL
  hal::relay().set(index, on ? hal::RelayState::On : hal::RelayState::Off);
#endif

  saveToRegistry();
}

bool get(uint8_t index) {
  if (index >= kOutletCount) return false;
  return g_state[index] != 0;
}

uint8_t count() {
  return kOutletCount;
}

} // namespace app::outlets
