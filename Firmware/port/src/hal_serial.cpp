#include <Arduino.h>
#include "hal_serial.h"

namespace hal {

  static HardwareSerial& refUart(Uart uart) {
    switch (uart) {
      case Uart::Uart0: return Serial;
      case Uart::Uart1: return Serial1;
      case Uart::Uart2: return Serial2;
      case Uart::Uart3: return Serial3;
      default:          return Serial;
    }
  }

  void serialIniciar(Uart uart, uint32_t baud) {
    static bool iniciado[4] = {false, false, false, false};
    const uint8_t idx = (uint8_t)uart;
    if (idx < 4 && !iniciado[idx]) {
      refUart(uart).begin(baud);
      iniciado[idx] = true;
    }
  }

  void serialEscribir(Uart uart, const char* s) {
    refUart(uart).print(s);
  }

  void serialEscribirLinea(Uart uart, const char* s) {
    refUart(uart).println(s);
  }

  void serialEscribirU32(Uart uart, uint32_t v) {
    refUart(uart).print(v);
  }

  void serialEscribirLineaU32(Uart uart, uint32_t v) {
    refUart(uart).println(v);
  }

}
