#pragma once
#include <Arduino.h>

namespace hal {

  inline void delayMs(uint32_t ms) {
    delay(ms);
  }

  inline uint32_t millisNow() {
    return millis();
  }

}
