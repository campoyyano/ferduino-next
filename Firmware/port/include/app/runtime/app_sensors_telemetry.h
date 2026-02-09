#pragma once

#include <stdint.h>

namespace app::runtime {

// Publica temperaturas fake para debug.
// Intervalo en segundos.
void publishTempsLoop(uint32_t intervalSec);

} // namespace app::runtime
