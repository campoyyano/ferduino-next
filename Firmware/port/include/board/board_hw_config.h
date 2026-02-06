#pragma once
#include <stdint.h>

// =====================================================
// Config HW del PORT (ajustable sin tocar el código)
// =====================================================
//
// IMPORTANTE:
// - PCF8575: dirección I2C depende de A0/A1/A2.
// - Polarity: depende de cómo esté cableado (relés/transistores).
//
// En caso de duda:
// - Deja los valores por defecto para COMPILAR.
// - Cuando tengamos el esquemático, fijamos definitivamente.
//

#ifndef FERDUINO_PCF8575_ADDR
  // Placeholder común para PCF8575 (AJUSTAR cuando confirmemos por esquemático)
  #define FERDUINO_PCF8575_ADDR 0x20
#endif

#ifndef FERDUINO_PCF8575_ACTIVE_LOW
  // 1 = escribir 0 "activa" la salida (muy común con relés/transistores)
  // 0 = escribir 1 "activa"
  #define FERDUINO_PCF8575_ACTIVE_LOW 1
#endif
