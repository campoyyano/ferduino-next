#pragma once

#include <stdint.h>

namespace app::runtime {

// Publica telemetría tempctrl cada N segundos (si MQTT conectado).
void tempctrlTelemetryLoop(uint32_t everySeconds);

} // namespace app::runtime
