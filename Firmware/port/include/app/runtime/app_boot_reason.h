#pragma once

#include <stdint.h>

namespace app::runtime {

// Devuelve el valor bruto de MCUSR capturado al arranque (AVR).
uint8_t bootMcusr();

// Devuelve un string estable con la causa principal del reset.
const char* bootReason();

} // namespace app::runtime
