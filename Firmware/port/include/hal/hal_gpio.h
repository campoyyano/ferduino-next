#pragma once
#include <Arduino.h>

// HAL GPIO - versión mínima
// Objetivo: encapsular pinMode/digitalWrite/digitalRead para facilitar portabilidad.

namespace hal {

  enum class PinMode : uint8_t {
    Entrada = INPUT,
    EntradaPullup = INPUT_PULLUP,
    Salida = OUTPUT
  };

  inline void modoPin(uint8_t pin, PinMode modo) {
    pinMode(pin, static_cast<uint8_t>(modo));
  }

  inline void escribirPin(uint8_t pin, bool nivel_alto) {
    digitalWrite(pin, nivel_alto ? HIGH : LOW);
  }

  inline bool leerPin(uint8_t pin) {
    return (digitalRead(pin) == HIGH);
  }

}
