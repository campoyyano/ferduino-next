#include <Arduino.h>
//#include "ferduino_original_config.h" // pines originales
#include "hal_gpio.h"
#include "hal_time.h"
#include "pins.h"

// Objetivo:
// - Probar que podemos usar los pines del firmware original
// - Pero accediendo a HW a través de HAL
// - Sin implementar lógica del acuario todavía

// Test mínimo sin depender del pinout Ferduino.
// LED onboard del Mega: D13
static const uint8_t PIN_TEST = 13;

void app_gpio_smoketest_run() {
  hal::modoPin(PIN_TEST, hal::PinMode::Salida);

  hal::escribirPin(PIN_TEST, true);
  hal::delayMs(200);

  hal::escribirPin(PIN_TEST, false);
  hal::delayMs(800);
}
