#pragma once

// Modos soportados (valores numéricos para usar en #if)
#define FERDUINO_COMMS_LEGACY 0
#define FERDUINO_COMMS_HA     1

// Selección por compilación (PlatformIO build_flags):
//   -D FERDUINO_COMMS_MODE=FERDUINO_COMMS_LEGACY
//   -D FERDUINO_COMMS_MODE=FERDUINO_COMMS_HA
#ifndef FERDUINO_COMMS_MODE
  #define FERDUINO_COMMS_MODE FERDUINO_COMMS_LEGACY
#endif
