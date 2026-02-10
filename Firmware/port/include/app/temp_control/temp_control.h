#pragma once

#include <stdint.h>

namespace app::tempctrl {

// Configuración efectiva del control (todo en décimas de °C, x10)
struct Config {
  int16_t set_x10 = 255;   // 25.5°C
  int16_t off_x10 = 5;     // 0.5°C
  int16_t alarm_x10 = 10;  // 1.0°C
};

// Estado del controlador
struct State {
  bool valid = false;

  // Señales internas
  bool alarm_active = false;
  bool heater_on = false;
  bool chiller_on = false;

  // Diagnóstico
  int16_t water_x10 = 0;
  Config cfg{};
};

// Inicializa y carga configuración (keys 100..102) desde NVM registry.
void begin();

// Ejecuta el control (usa sensores FAKE/HW según módulo sensors).
// Internamente limita la frecuencia de ejecución.
void loop();

// Último estado calculado.
State last();

} // namespace app::tempctrl
