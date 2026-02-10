#pragma once

#include <stdint.h>

namespace app::outlets {

// idx: 0..8 representa outlets 1..9
void begin();
// Manual set: fuerza el outlet a modo manual (deshabilita auto)
void set(uint8_t idx, bool state);
bool get(uint8_t idx);

// Auto arbitration (scheduler)
void setAuto(uint8_t idx, bool enabled);
bool isAuto(uint8_t idx);

// Aplica estado deseado solo si el outlet está en auto.
// Devuelve true si se cambió el estado del outlet.
bool applyDesiredIfAuto(uint8_t idx, bool desiredOn);

uint8_t count();

} // namespace app::outlets
