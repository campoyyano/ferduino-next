#pragma once
#include <stdint.h>
#include <stddef.h>

namespace hal {

  // Inicia I2C si no está inicializado (idempotente).
  void i2cIniciar();

  // Escribe len bytes a addr. Devuelve true si OK.
  bool i2cEscribir(uint8_t addr, const uint8_t* data, size_t len);

  // Lee len bytes desde addr. Devuelve true si OK.
  bool i2cLeer(uint8_t addr, uint8_t* data, size_t len);

}
