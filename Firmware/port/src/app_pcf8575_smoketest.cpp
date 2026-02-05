#include <Arduino.h>

#include "hal_time.h"
#include "hal_pcf8575.h"
#include "board_hw_config.h"
#include "pins.h"

// Smoketest A5:
// - Inicializa PCF8575
// - Activa una salida PCF cada vez (P0..P12)
// - Sin lógica de acuario todavía

static hal::PCF8575 pcf(FERDUINO_PCF8575_ADDR);

static void activarSolo(uint8_t p) {
  // Apagamos P0..P12
  for (uint8_t i = 0; i <= 12; i++) {
    pcf.writePin(i, false);
  }
  // Activamos solo p
  pcf.writePin(p, true);
}

void app_pcf8575_smoketest_run() {
  static bool init = false;
  if (!init) {
    pcf.begin();
    // Asegurar todo apagado al inicio
    for (uint8_t i = 0; i <= 12; i++) {
      pcf.writePin(i, false);
    }
    init = true;
  }

  // Secuencia P0..P12
  for (uint8_t p = 0; p <= 12; p++) {
    activarSolo(p);
    hal::delayMs(400);
  }

  // Fin de ciclo: todo off
  for (uint8_t i = 0; i <= 12; i++) {
    pcf.writePin(i, false);
  }
  hal::delayMs(800);
}
