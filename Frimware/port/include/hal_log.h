#pragma once
#include <Arduino.h>

namespace hal {

  // Inicializa el log (por ahora usamos Serial)
  inline void logInit(uint32_t baud = 115200) {
    Serial.begin(baud);
  }

  inline void logPrint(const char* s) {
    Serial.print(s);
  }

  inline void logPrintln(const char* s) {
    Serial.println(s);
  }

  inline void logPrint(int v) {
    Serial.print(v);
  }

  inline void logPrintln(int v) {
    Serial.println(v);
  }

}
