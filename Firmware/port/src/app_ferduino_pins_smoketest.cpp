#include <Arduino.h>

#include "hal_gpio.h"
#include "hal_time.h"
#include "pins.h"

// Smoketest A3:
// - Usa pines REALES del Ferduino (pero a través de pins.h del PORT)
// - Accede al HW por HAL
// - No arrastra Configuration.h ni librerías legacy

void app_ferduino_pins_smoketest_run() {
  hal::modoPin(pins::ledPinUV, hal::PinMode::Salida);
  hal::modoPin(pins::ledPinMoon, hal::PinMode::Salida);

  // Parpadeo alterno UV/Moon
  hal::escribirPin(pins::ledPinUV, true);
  hal::escribirPin(pins::ledPinMoon, false);
  hal::delayMs(300);

  hal::escribirPin(pins::ledPinUV, false);
  hal::escribirPin(pins::ledPinMoon, true);
  hal::delayMs(300);
}
