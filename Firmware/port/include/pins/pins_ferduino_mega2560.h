#pragma once
#include <Arduino.h>

// =====================================================
// Pinmap BASE Ferduino - Arduino Mega 2560
// Fuente directa: Configuration.h del proyecto original
// =====================================================

namespace pins_base {

// ---------------------
// Control general
// ---------------------
static const uint8_t alarmPin          = 0;
static const uint8_t desativarFanPin   = 1;

// ---------------------
// Canales LED (PWM)
// ---------------------
static const uint8_t ledPinUV      = 8;
static const uint8_t ledPinBlue    = 9;
static const uint8_t ledPinWhite   = 10;
static const uint8_t ledPinRoyBlue = 11;
static const uint8_t ledPinRed     = 12;
static const uint8_t fanPin        = 13;

// ---------------------
// Multiplexador stamps
// ---------------------
static const uint8_t multiplexadorS0Pin = 16;
static const uint8_t multiplexadorS1Pin = 17;

// ---------------------
// Relés / cargas AC
// ---------------------
static const uint8_t aquecedorPin = 42;
static const uint8_t chillerPin   = 43;
static const uint8_t ledPinMoon   = 44;
static const uint8_t wavemaker1   = 45;
static const uint8_t wavemaker2   = 46;

// ---------------------
// Sensores
// ---------------------
static const uint8_t sensoresPin = 49;

// ---------------------
// Sensores analógicos
// ---------------------
static const uint8_t sensor1 = 54; // A0
static const uint8_t sensor2 = 55; // A1
static const uint8_t sensor3 = 56; // A2
static const uint8_t sensor4 = 57; // A3
static const uint8_t sensor5 = 58; // A4
static const uint8_t sensor6 = 59; // A5

// ---------------------
// Salidas PCF8575 (mapeadas como P0..P12)
// ---------------------
static const uint8_t ozonizadorPin  = 0;
static const uint8_t reatorPin      = 1;
static const uint8_t bomba1Pin      = 2;
static const uint8_t bomba2Pin      = 3;
static const uint8_t bomba3Pin      = 4;
static const uint8_t temporizador1  = 5;
static const uint8_t temporizador2  = 6;
static const uint8_t temporizador3  = 7;
static const uint8_t temporizador4  = 8;
static const uint8_t temporizador5  = 9;
static const uint8_t solenoide1Pin  = 10;
static const uint8_t alimentadorPin = 11;
static const uint8_t skimmerPin     = 12;

} // namespace pins_base
