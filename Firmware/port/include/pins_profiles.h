#pragma once

// ===============================
// Perfiles de pines (Mega 2560)
// ===============================
//
// Objetivo:
// - Perfil LEGACY: compatible con el firmware original (usa D0/D1)
// - Perfil FASE2: libera Serial0 (D0/D1) usando pines alternativos (D14/D15)
//
// Cómo usar:
// - Dejar por defecto LEGACY al inicio.
// - Cambiar a FASE2 cuando se use la nueva shield.
//
// Nota: Estos perfiles NO cambian el resto del pinmap (LEDs, wavemakers, etc.),
// solo mueven alarmPin y desativarFanPin.

#define PINS_PROFILE_LEGACY 1
#define PINS_PROFILE_FASE2  2

#ifndef PINS_PROFILE
  // Por defecto: compatibilidad total con el diseño original
  #define PINS_PROFILE PINS_PROFILE_LEGACY
#endif

// -------------------------------
// Definición de pines por perfil
// -------------------------------
#if (PINS_PROFILE == PINS_PROFILE_LEGACY)

  // Perfil original (ojo: D0/D1 = Serial0)
  static const uint8_t alarmPin          = 0; // D0
  static const uint8_t desativarFanPin   = 1; // D1

#elif (PINS_PROFILE == PINS_PROFILE_FASE2)

  // Perfil Fase 2 (libera Serial0)
  static const uint8_t alarmPin          = 14; // D14
  static const uint8_t desativarFanPin   = 15; // D15

#else
  #error "PINS_PROFILE invalido. Usa PINS_PROFILE_LEGACY o PINS_PROFILE_FASE2."
#endif
