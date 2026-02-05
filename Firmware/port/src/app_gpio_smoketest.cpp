#include <Arduino.h>
//#include "ferduino_original_config.h" // pines originales
#include "hal_gpio.h"
#include "hal_time.h"

// Objetivo:
// - Probar que podemos usar los pines del firmware original
// - Pero accediendo a HW a través de HAL
// - Sin implementar lógica del acuario todavía

static void configurarSalidasBasicas() {
  // Ejemplo: un par de salidas conocidas (LEDs) y moonlight
  hal::modoPin(ledPinUV, hal::PinMode::Salida);
  hal::modoPin(ledPinMoon, hal::PinMode::Salida);
}

static void parpadeoPrueba() {
  hal::escribirPin(ledPinUV, true);
  hal::delayMs(200);
  hal::escribirPin(ledPinUV, false);
  hal::delayMs(800);
}

void app_gpio_smoketest_run() {
  configurarSalidasBasicas();
  parpadeoPrueba();
}
