#include <Arduino.h>

#include "hal/hal_time.h"
#include "hal/hal_ioexpander.h"
#include "hal/hal_serial.h"
#include "board/board_debug.h"
#include "board/board_hw_config.h"

static hal::IoExpander iox(FERDUINO_PCF8575_ADDR);

static void logLinea(const char* s) {
  hal::serialEscribirLinea(board::debugUart, s);
}

void app_ioexpander_smoketest_run() {
  static bool init = false;

  if (!init) {
    hal::serialIniciar(board::debugUart, 115200);

    if (!iox.begin()) {
      logLinea("[IOX] begin() FALLO (I2C no responde)");
    } else {
      logLinea("[IOX] begin() OK");
    }

    // Apagamos todo al arranque (lógico). La HAL ya invierte si ACTIVE_LOW=1
    iox.writeAll(0xFFFF);   // estado "alto" en todas las líneas (típico idle en PCF)
    for (uint8_t p = 0; p < 16; p++) {
      iox.writePin(p, false);
    }

    init = true;
  }

  // Recorremos P0..P12 (típicos relés)
  for (uint8_t p = 0; p <= 12; p++) {
    for (uint8_t k = 0; k <= 12; k++) {
      iox.writePin(k, false);
    }

    iox.writePin(p, true);

    uint16_t raw = 0;
    if (iox.readAll(raw)) {
      hal::serialEscribir(board::debugUart, "[IOX] raw=0x");
      char buf[8];
      ultoa((unsigned long)raw, buf, 16);
      hal::serialEscribirLinea(board::debugUart, buf);
    } else {
      logLinea("[IOX] readAll FALLO");
    }

    hal::delayMs(350);
  }

  for (uint8_t p = 0; p < 16; p++) {
    iox.writePin(p, false);
  }
  hal::delayMs(800);
}
