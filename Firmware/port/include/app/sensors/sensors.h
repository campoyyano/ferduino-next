#pragma once

#include <stdint.h>

namespace app::sensors {

struct Temperatures {
  float water_c;     // ºC
  float air_c;       // ºC
  bool water_valid;
  bool air_valid;
  uint32_t ts_ms;    // timestamp (millis) de la última actualización
};

void begin();
void loop();
Temperatures last();

} // namespace app::sensors
