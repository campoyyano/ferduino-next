#include "app/comms/ha/ha_outlet_control.h"

#include <string.h>

// Aquí dejaremos el “punto de integración” a HAL relés cuando exista en runtime.
// Por ahora: estado simulado en RAM para permitir desarrollo sin placa.

namespace app::ha {

static uint8_t s_outlet[10] = {0}; // 1..9

void applyOutletCommand(uint8_t outletNumber, uint8_t value) {
  if (outletNumber < 1 || outletNumber > 9) return;
  s_outlet[outletNumber] = value ? 1 : 0;

  // TODO(B3/B4): aquí conectar con hal::relay().set(outletNumber, value) o mapping.
}

uint8_t getOutletState(uint8_t outletNumber) {
  if (outletNumber < 1 || outletNumber > 9) return 0;
  return s_outlet[outletNumber];
}

} // namespace app::ha
