
#include <Arduino.h>
#include <Wire.h>
#include "hal/hal_i2c.h"

namespace hal {

  void i2cIniciar() {
    static bool iniciado = false;
    if (!iniciado) {
      Wire.begin();
      iniciado = true;
    }
  }

  bool i2cEscribir(uint8_t addr, const uint8_t* data, size_t len) {
    i2cIniciar();
    Wire.beginTransmission(addr);
    for (size_t i = 0; i < len; i++) {
      Wire.write(data[i]);
    }
    const uint8_t err = Wire.endTransmission();
    return (err == 0);
  }

  bool i2cLeer(uint8_t addr, uint8_t* data, size_t len) {
    i2cIniciar();
    const size_t n = Wire.requestFrom((int)addr, (int)len);
    if (n != len) {
      // Vaciar lo que haya llegado
      while (Wire.available()) (void)Wire.read();
      return false;
    }
    for (size_t i = 0; i < len; i++) {
      if (!Wire.available()) return false;
      data[i] = (uint8_t)Wire.read();
    }
    return true;
  }

}
