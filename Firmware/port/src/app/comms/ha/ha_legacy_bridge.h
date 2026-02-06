#pragma once

#include <stddef.h>

namespace app::ha {

// Parsea JSON legacy (ID0 Home) y publica HA state derivado a:
//   ferduino/<device_id>/state
//
// Diseñado para AVR: sin parser JSON, extracción por claves conocidas.
void bridgeFromLegacyHomeJson(const char* legacyJson);

} // namespace app::ha
