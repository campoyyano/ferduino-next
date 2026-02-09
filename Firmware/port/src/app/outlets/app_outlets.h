#pragma once

#include <stdint.h>

namespace app::outlets {

// idx: 0..8 representa outlets 1..9
void begin();
void set(uint8_t idx, bool state);
bool get(uint8_t idx);

} // namespace app::outlets
