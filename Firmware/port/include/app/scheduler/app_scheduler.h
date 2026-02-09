#pragma once

#include <stdint.h>

namespace app::scheduler {

struct TimeHM {
  uint8_t hour;   // 0..23
  uint8_t minute; // 0..59
};

enum class TimeSource : uint8_t {
  Millis = 0,
  Rtc    = 1
};

// Inicializa el scheduler.
void begin();

// Llamar frecuentemente desde runtime loop.
// Detecta cambios de minuto (en base a minuteOfDay()).
void loop();

// Hora/minuto actual.
TimeHM now();

// Minuto del día 0..1439 (origen: millis o RTC según configuración)
uint16_t minuteOfDay();

// Minutos desde boot (monótono, basado en millis()).
// Solo para debug/telemetría (no usar para lógica funcional).
uint32_t minutesSinceBoot();

// Tick de cambio de minuto:
// - minuteTick() -> peek (no consume)
// - consumeMinuteTick() -> consume/reset
bool minuteTick();
bool consumeMinuteTick();

// Indica la fuente de tiempo efectiva.
TimeSource timeSource();

} // namespace app::scheduler
