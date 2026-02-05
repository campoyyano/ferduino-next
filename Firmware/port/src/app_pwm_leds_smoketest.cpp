#include <Arduino.h>

#include "hal_gpio.h"
#include "hal_time.h"
#include "hal_pwm.h"
#include "pins.h"

static void configurarPwmLeds() {
  hal::modoPin(pins::ledPinUV,      hal::PinMode::Salida);
  hal::modoPin(pins::ledPinBlue,    hal::PinMode::Salida);
  hal::modoPin(pins::ledPinWhite,   hal::PinMode::Salida);
  hal::modoPin(pins::ledPinRoyBlue, hal::PinMode::Salida);
  hal::modoPin(pins::ledPinRed,     hal::PinMode::Salida);
  hal::modoPin(pins::ledPinMoon,    hal::PinMode::Salida);
}

static void apagarTodo() {
  hal::pwmEscribir(pins::ledPinUV,      0);
  hal::pwmEscribir(pins::ledPinBlue,    0);
  hal::pwmEscribir(pins::ledPinWhite,   0);
  hal::pwmEscribir(pins::ledPinRoyBlue, 0);
  hal::pwmEscribir(pins::ledPinRed,     0);
  hal::pwmEscribir(pins::ledPinMoon,    0);
}

// Fade 0->255->0 en un pin, manteniendo los demás apagados
static void rampaEnPin(uint8_t pin, uint16_t paso_ms) {
  apagarTodo();
  for (int v = 0; v <= 255; v++) {
    hal::pwmEscribir(pin, (uint8_t)v);
    hal::delayMs(paso_ms);
  }
  for (int v = 255; v >= 0; v--) {
    hal::pwmEscribir(pin, (uint8_t)v);
    hal::delayMs(paso_ms);
  }
}

void app_pwm_leds_smoketest_run() {
  static bool init = false;
  if (!init) {
    configurarPwmLeds();
    apagarTodo();
    init = true;
  }

  // Orden: UV -> Blue -> White -> RoyalBlue -> Red -> Moon
  rampaEnPin(pins::ledPinUV,      5);
  rampaEnPin(pins::ledPinBlue,    5);
  rampaEnPin(pins::ledPinWhite,   5);
  rampaEnPin(pins::ledPinRoyBlue, 5);
  rampaEnPin(pins::ledPinRed,     5);
  rampaEnPin(pins::ledPinMoon,    5);
}
