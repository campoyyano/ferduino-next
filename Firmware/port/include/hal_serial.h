#pragma once
#include <stdint.h>

namespace hal {

  enum class Uart : uint8_t {
    Uart0 = 0,  // Serial
    Uart1 = 1,  // Serial1
    Uart2 = 2,  // Serial2
    Uart3 = 3   // Serial3
  };

  // Inicializa un UART a baudrate (idempotente)
  void serialIniciar(Uart uart, uint32_t baud);

  // Escritura básica
  void serialEscribir(Uart uart, const char* s);
  void serialEscribirLinea(Uart uart, const char* s);

  // Para números simples (evitamos String)
  void serialEscribirU32(Uart uart, uint32_t v);
  void serialEscribirLineaU32(Uart uart, uint32_t v);

}
