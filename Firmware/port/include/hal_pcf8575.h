#pragma once
#include <stdint.h>

namespace hal {

  class PCF8575 {
  public:
    explicit PCF8575(uint8_t addr);

    // Inicializa el estado interno (pone todo en "desactivado").
    // Nota: en PCF8575 escribir '1' deja el pin en high/entrada (según carga).
    bool begin();

    // Escribe un pin 0..15 con valor lógico "activo" (true) / "inactivo" (false)
    bool writePin(uint8_t pin, bool activo);

    // Lee el estado actual del pin 0..15 (nivel leído del PCF).
    bool readPin(uint8_t pin, bool& nivel);

    // Escribe el registro completo (16 bits) ya con polaridad aplicada.
    bool writeAll(uint16_t valueRaw);

    // Lee el registro completo.
    bool readAll(uint16_t& valueRaw);

  private:
    uint8_t _addr;
    uint16_t _shadow; // último valor escrito (raw, antes de leer)

    bool flush();
    static bool pinValida(uint8_t pin) { return pin < 16; }
  };

}
