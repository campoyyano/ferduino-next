#pragma once
#include "hal_serial.h"

// UART de debug configurable por build_flags
// 0=Serial, 1=Serial1, 2=Serial2, 3=Serial3
#ifndef DEBUG_UART
  #define DEBUG_UART 0
#endif

namespace board {
  static constexpr hal::Uart debugUart =
    (DEBUG_UART == 1) ? hal::Uart::Uart1 :
    (DEBUG_UART == 2) ? hal::Uart::Uart2 :
    (DEBUG_UART == 3) ? hal::Uart::Uart3 :
                        hal::Uart::Uart0;
}
