#include "hal_ioexpander.h"
#include "hal_i2c.h"
#include "board_hw_config.h"

namespace hal {

  static inline bool onToPhysicalLevel(bool on) {
#if FERDUINO_PCF8575_ACTIVE_LOW
    return !on;
#else
    return on;
#endif
  }

  IoExpander::IoExpander(uint8_t addr)
    : _addr(addr), _shadow(0xFFFF)
  {}

  bool IoExpander::begin() {
    _shadow = 0xFFFF;
    return flush();
  }

  bool IoExpander::flush() {
    uint8_t buf[2];
    buf[0] = (uint8_t)(_shadow & 0xFF);
    buf[1] = (uint8_t)((_shadow >> 8) & 0xFF);
    return hal::i2cEscribir(_addr, buf, 2);
  }

  bool IoExpander::writePin(uint8_t pin, bool on) {
    if (!pinOk(pin)) return false;

    const bool level = onToPhysicalLevel(on);
    if (level) _shadow |=  (uint16_t)(1u << pin);
    else       _shadow &= (uint16_t)~(1u << pin);

    return flush();
  }

  bool IoExpander::readAll(uint16_t& raw) {
    uint8_t buf[2] = {0, 0};
    if (!hal::i2cLeer(_addr, buf, 2)) return false;
    raw = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
    return true;
  }

  bool IoExpander::readPin(uint8_t pin, bool& nivel) {
    if (!pinOk(pin)) return false;
    uint16_t raw = 0;
    if (!readAll(raw)) return false;
    nivel = ((raw >> pin) & 0x1) != 0;
    return true;
  }

  bool IoExpander::writeAll(uint16_t raw) {
    _shadow = raw;
    return flush();
  }

}
