#include "app/sensors/sensors.h"

#include <Arduino.h>

#include "app/app_build_flags.h"

namespace app::sensors {

#if APP_SENSORS_USE_HW == 0

static Temperatures g_last{};

static float slowVariation(float base, uint32_t ms, float amplitude, uint32_t period_ms) {
  // Variación triangular determinista (sin sin/cos)
  const uint32_t t = (period_ms == 0) ? 0 : (ms % period_ms);
  const uint32_t half = period_ms / 2;
  float x = 0.0f;

  if (half == 0) {
    x = 0.0f;
  } else if (t <= half) {
    x = (float)t / (float)half;           // 0..1
  } else {
    x = 1.0f - ((float)(t - half) / (float)half); // 1..0
  }

  // map 0..1 a -1..+1
  const float s = (x * 2.0f) - 1.0f;
  return base + (s * amplitude);
}

void begin() {
  const uint32_t now = millis();
  g_last.water_c = 25.1f;
  g_last.air_c = 24.7f;
  g_last.water_valid = true;
  g_last.air_valid = true;
  g_last.ts_ms = now;
}

void loop() {
  const uint32_t now = millis();

  // Actualización cada ~1s para mantener valores vivos sin coste
  static uint32_t lastUpdate = 0;
  if (lastUpdate != 0 && (uint32_t)(now - lastUpdate) < 1000UL) return;
  lastUpdate = now;

  // Water ~25.1 ±0.15 (periodo 5 min)
  g_last.water_c = slowVariation(25.1f, now, 0.15f, 300000UL);

  // Air ~24.7 ±0.12 (periodo 7 min)
  g_last.air_c = slowVariation(24.7f, now, 0.12f, 420000UL);

  g_last.water_valid = true;
  g_last.air_valid = true;
  g_last.ts_ms = now;
}

Temperatures last() {
  return g_last;
}

#else  // APP_SENSORS_USE_HW == 1

// Backend HW todavía no implementado en la migración B6.
// Este stub existe para que el cambio de flag NO rompa compilación.
// Cuando haya hardware, sustituir por drivers/HAL reales y mantener la misma API.

static Temperatures g_last{};

void begin() {
  g_last.water_c = 0.0f;
  g_last.air_c = 0.0f;
  g_last.water_valid = false;
  g_last.air_valid = false;
  g_last.ts_ms = millis();
}

void loop() {
  // TODO(HW): leer sensores reales
  g_last.ts_ms = millis();
}

Temperatures last() {
  return g_last;
}

#endif

} // namespace app::sensors
