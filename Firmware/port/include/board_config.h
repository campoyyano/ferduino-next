#pragma once

// ===============================
// Configuración de placa / perfil
// ===============================
//
// Objetivo:
// - Centralizar selección de placa y perfil de pines.
// - Evitar #defines repartidos por el código.
// - Facilitar portabilidad Mega / Due / Giga.
//
// Por defecto dejamos Mega + LEGACY (cero sorpresas).

// ----- Selección de placa objetivo -----
#define BOARD_MEGA2560 1
#define BOARD_DUE      2
#define BOARD_GIGA     3

#ifndef BOARD_TARGET
  #define BOARD_TARGET BOARD_MEGA2560
#endif

// ----- Selección de perfil de pines -----
#define PINS_PROFILE_LEGACY 1
#define PINS_PROFILE_FASE2  2

#ifndef PINS_PROFILE
  #define PINS_PROFILE PINS_PROFILE_LEGACY
#endif

// ----- Parámetros derivados (para HAL) -----
#if (BOARD_TARGET == BOARD_MEGA2560)
  // Mega: lógica 5V, pero nuestra shield será 3.3V en periféricos
  static const bool boardEs5V = true;
#elif (BOARD_TARGET == BOARD_DUE)
  static const bool boardEs5V = false;
#elif (BOARD_TARGET == BOARD_GIGA)
  static const bool boardEs5V = false;
#else
  #error "BOARD_TARGET inválido"
#endif
