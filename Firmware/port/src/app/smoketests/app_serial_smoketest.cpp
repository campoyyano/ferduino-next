#include <Arduino.h>
#include "hal/hal_time.h"
#include "hal/hal_serial.h"
#include "board/board_debug.h"

void app_serial_smoketest_run() {
  static bool init = false;
  if (!init) {
    hal::serialIniciar(board::debugUart, 115200);
    hal::serialEscribirLinea(board::debugUart, "[A6] Serial smoketest OK");
    init = true;
  }

  // “heartbeat” cada ~1s
  hal::serialEscribir(board::debugUart, "[A6] millis=");
  hal::serialEscribirLineaU32(board::debugUart, (uint32_t)millis());
  hal::delayMs(1000);
}
