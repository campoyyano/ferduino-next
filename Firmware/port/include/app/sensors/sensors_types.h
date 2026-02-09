#pragma once

#include <stdint.h>

namespace app::sensors {

struct TemperatureReading {
  int16_t celsius_x10 = 0;   // 251 = 25.1°C
  bool valid = false;
};

struct Temperatures {
  TemperatureReading water;
  TemperatureReading air;
  uint32_t ts_ms = 0;        // millis() cuando se tomó la lectura
};

} // namespace app::sensors
