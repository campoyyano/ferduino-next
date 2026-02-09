#include "app/outlets/app_outlets.h"

#include "app/nvm/eeprom_registry.h"

// Por defecto: modo stub (sin hardware)
#ifndef APP_OUTLETS_USE_RELAY_HAL
#define APP_OUTLETS_USE_RELAY_HAL 0
#endif

#if APP_OUTLETS_USE_RELAY_HAL
#include "hal/hal_relay.h"
#endif

namespace app::outlets {

static bool s_state[9] = {false, false, false, false, false, false, false, false, false};

// Keys TLV (según hoja de ruta):
// 311..319 → estado outlets (U32 0/1)
static constexpr uint16_t kOutletStateBaseKey = 311;

#if APP_OUTLETS_USE_RELAY_HAL
// Mapeo provisional idx(0..8) -> relé físico (primeros 9 del enum).
static hal::Relay mapOutletIdxToRelay(uint8_t idx)
{
  switch (idx) {
    case 0: return hal::Relay::Ozonizador;
    case 1: return hal::Relay::Reator;
    case 2: return hal::Relay::Bomba1;
    case 3: return hal::Relay::Bomba2;
    case 4: return hal::Relay::Bomba3;
    case 5: return hal::Relay::Temporizador1;
    case 6: return hal::Relay::Temporizador2;
    case 7: return hal::Relay::Temporizador3;
    case 8: return hal::Relay::Temporizador4;
    default: return hal::Relay::Ozonizador;
  }
}
#endif

void begin()
{
  auto& reg = app::nvm::registry();

#if APP_OUTLETS_USE_RELAY_HAL
  (void)hal::relayInit();
#endif

  for (uint8_t i = 0; i < 9; i++) {
    uint32_t v = 0;
    (void)reg.getU32(static_cast<uint16_t>(kOutletStateBaseKey + i), v);
    s_state[i] = (v != 0);

#if APP_OUTLETS_USE_RELAY_HAL
    (void)hal::relaySet(mapOutletIdxToRelay(i), s_state[i]);
#endif
  }
}

void set(uint8_t idx, bool state)
{
  if (idx >= 9) return;
  if (s_state[idx] == state) return;

  s_state[idx] = state;

#if APP_OUTLETS_USE_RELAY_HAL
  (void)hal::relaySet(mapOutletIdxToRelay(idx), s_state[idx]);
#endif

  auto& reg = app::nvm::registry();
  reg.setU32(static_cast<uint16_t>(kOutletStateBaseKey + idx), s_state[idx] ? 1u : 0u);
}

bool get(uint8_t idx)
{
  if (idx >= 9) return false;
  return s_state[idx];
}

} // namespace app::outlets
