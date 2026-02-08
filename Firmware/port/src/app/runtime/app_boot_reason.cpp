#include "app/runtime/app_boot_reason.h"

#if defined(ARDUINO_ARCH_AVR)
  #include <avr/io.h>
#endif

namespace app::runtime {

#if defined(ARDUINO_ARCH_AVR)
// Captura temprana: inicialización estática antes de setup().
static uint8_t g_mcusr = MCUSR;
#else
static uint8_t g_mcusr = 0;
#endif

uint8_t bootMcusr() {
  return g_mcusr;
}

const char* bootReason() {
#if defined(ARDUINO_ARCH_AVR)
  // Bits típicos: PORF, EXTRF, BORF, WDRF, JTRF (según MCU)
  // Priorizamos causas “fuertes” primero.
  if (g_mcusr & (1u << WDRF))  return "watchdog";
  if (g_mcusr & (1u << BORF))  return "brown_out";
  if (g_mcusr & (1u << EXTRF)) return "external";
  if (g_mcusr & (1u << PORF))  return "power_on";
  #ifdef JTRF
  if (g_mcusr & (1u << JTRF))  return "jtag";
  #endif
  return "unknown";
#else
  return "unknown";
#endif
}

} // namespace app::runtime
