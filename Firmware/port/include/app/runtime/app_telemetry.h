#pragma once

#include <stdint.h>

namespace app::runtime {

// Llamar en loop; publica uptime y scheduler debug cada intervalo (segundos).
void telemetryLoop(uint32_t intervalSec);

} // namespace app::runtime
