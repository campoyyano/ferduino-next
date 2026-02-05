#pragma once
#include <stdint.h>

namespace hal {

  class IoExpander {
  public:
    explicit IoExpander(uint8_t addr);

    bool begin();
    bool writePin(uint8_t pin, bool on);
    bool readPin(uint8_t pin, bool& nivel);

    bool writeAll(uint16_t raw);
    bool readAll(uint16_t& raw);

  private:
    uint8_t _addr;
    uint16_t _shadow;

    bool flush();
    static bool pinOk(uint8_t pin) { return pin < 16; }
  };

}
