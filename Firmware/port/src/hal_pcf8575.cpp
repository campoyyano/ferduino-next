#include "hal_pcf8575.h"
#include "hal_i2c.h"
#include "board_hw_config.h"

namespace hal {

  static inline uint16_t aplicarPolaridad(uint16_t raw) {
#if FERDUINO_PCF8575_ACTIVE_LOW
    // Si activo-bajo, invertimos el significado lógico externo vs nivel físico.
    // Aquí asumimos "raw" representa niveles físicos.
    return raw;
#else
    return raw;
#endif
  }

  static inline bool aplicarPolaridadPin(bool activo) {
#if FERDUINO_PCF8575_ACTIVE_LOW
    // activo=true => nivel físico 0
    return !activo;
#else
    // activo=true => nivel físico 1
    return activo;
#endif
  }

  PCF8575::PCF8575(uint8_t addr)
    : _addr(addr), _shadow(0xFFFF) // por defecto todo a '1' (estado seguro típico)
  {}

  bool PCF8575::begin() {
    _shadow = 0xFFFF; // "todo desactivado" típico en PCF
    return flush();
  }

  bool PCF8575::flush() {
    // PCF8575 espera 2 bytes (LSB primero, luego MSB) en la mayoría de libs.
    uint8_t buf[2];
    buf[0] = (uint8_t)(_shadow & 0xFF);
    buf[1] = (uint8_t)((_shadow >> 8) & 0xFF);
    return i2cEscribir(_addr, buf, 2);
  }

  bool PCF8575::writePin(uint8_t pin, bool activo) {
    if (!pinValida(pin)) return false;

    const bool nivelFisico = aplicarPolaridadPin(activo);

    if (nivelFisico) _shadow |=  (uint16_t)(1u << pin);
    else             _shadow &= (uint16_t)~(1u << pin);

    return flush();
  }

  bool PCF8575::readAll(uint16_t& valueRaw) {
    uint8_t buf[2] = {0,0};
    if (!i2cLeer(_addr, buf, 2)) return false;
    valueRaw = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
    return true;
  }

  bool PCF8575::readPin(uint8_t pin, bool& nivel) {
    if (!pinValida(pin)) return false;
    uint16_t v = 0;
    if (!readAll(v)) return false;
    nivel = ((v >> pin) & 0x1) != 0;
    return true;
  }

  bool PCF8575::writeAll(uint16_t valueRaw) {
    _shadow = aplicarPolaridad(valueRaw);
    return flush();
  }

}
