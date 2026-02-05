#pragma once
#include <Arduino.h>

#include "pins_ferduino_mega2560.h"
#include "pins_profiles.h"

// Fachada: aquí exponemos SIEMPRE los mismos nombres (pins::X)
// Unifica: base + overrides del perfil

namespace pins {

  // ---------- base ----------
  // Cuando rellenemos pins_base::ledPinUV etc, los reexportamos aquí.
  // static const uint8_t ledPinUV   = pins_base::ledPinUV;
  // static const uint8_t ledPinMoon = pins_base::ledPinMoon;

   // -------- LEDs ----------
  static const uint8_t ledPinUV      = pins_base::ledPinUV;
  static const uint8_t ledPinBlue    = pins_base::ledPinBlue;
  static const uint8_t ledPinWhite   = pins_base::ledPinWhite;
  static const uint8_t ledPinRoyBlue = pins_base::ledPinRoyBlue;
  static const uint8_t ledPinRed     = pins_base::ledPinRed;
  static const uint8_t fanPin        = pins_base::fanPin;
  static const uint8_t ledPinMoon    = pins_base::ledPinMoon;

  // -------- perfil ----------
  static const uint8_t alarmPin        = ::alarmPin;
  static const uint8_t desativarFanPin = ::desativarFanPin;
} // namespace pins
