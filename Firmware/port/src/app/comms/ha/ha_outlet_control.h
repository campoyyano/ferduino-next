#pragma once

#include <stdint.h>

namespace app::ha {

// Aplica un comando HA a un outlet (1..9). value 0/1.
// En B2.3 no dependemos de placa: si no hay HAL relé real, se simula.
void applyOutletCommand(uint8_t outletNumber, uint8_t value);

// Lee el estado simulado/actual del outlet (1..9).
uint8_t getOutletState(uint8_t outletNumber);

} // namespace app::ha
