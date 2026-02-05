#pragma once
#include <stdint.h>

namespace hal {

  // PWM 8-bit: 0..255
  void pwmEscribir(uint8_t pin, uint8_t valor);

}
