#include <Arduino.h>
#include "hal_pwm.h"

namespace hal {

  void pwmEscribir(uint8_t pin, uint8_t valor) {
    // Asegúrate de haber hecho modoPin(pin, Salida) antes.
    analogWrite(pin, valor);
  }

}
