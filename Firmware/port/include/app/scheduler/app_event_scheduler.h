#pragma once

#include <stdint.h>

#include "app/scheduler/app_scheduler.h"

namespace app::scheduler::events {

// Modelo mínimo:
// - 1 ventana ON/OFF por canal (futuro: múltiples ventanas por canal).
// - Soporta ventana que cruza medianoche (on > off).
struct Window {
  uint16_t onMinute;   // 0..1439
  uint16_t offMinute;  // 0..1439
  bool enabled;
};

static constexpr uint8_t kMaxChannels = 9;

// Inicializa motor (no accede a HW).
void begin();

// Llamar desde runtime loop (reacciona a tick de minuto del scheduler base).
void loop();

// Configura ventana para un canal (0..8). on/off en formato HH:MM.
bool setWindow(uint8_t channel, app::scheduler::TimeHM on, app::scheduler::TimeHM off, bool enabled);

// Acceso a ventana actual.
Window window(uint8_t channel);

// Estado "deseado" calculado por el motor en el último loop.
bool desiredOn(uint8_t channel);

// Devuelve true si el estado deseado cambió desde el último consume.
bool consumeDesiredChanged(uint8_t channel);

// Calcula estado deseado para un minuto del día dado (helper puro, útil para tests).
bool computeDesired(const Window& w, uint16_t minuteOfDay);

} // namespace app::scheduler::events
