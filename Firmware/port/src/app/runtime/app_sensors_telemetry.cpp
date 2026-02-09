#include "app/runtime/app_sensors_telemetry.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#include "app/config/app_config.h"
#include "app/sensors/sensors.h"
#include "hal/hal_mqtt.h"

namespace app::runtime {

static uint32_t g_lastTempsMs = 0;

static int16_t toX10(float v) {
  // redondeo simétrico
  const float s = (v >= 0.0f) ? (v * 10.0f + 0.5f) : (v * 10.0f - 0.5f);
  return (int16_t)s;
}

void publishTempsLoop(uint32_t intervalSec) {
  if (!hal::mqtt().connected()) return;

  const auto& cfg = app::cfg::get();
  const char* deviceId = cfg.mqtt.deviceId;
  if (!deviceId || deviceId[0] == '\0') return;

  const uint32_t now = millis();
  const uint32_t intervalMs = intervalSec * 1000UL;
  if (g_lastTempsMs != 0 && (uint32_t)(now - g_lastTempsMs) < intervalMs) return;
  g_lastTempsMs = now;

  const app::sensors::Temperatures t = app::sensors::last();

  char topic[128];
  snprintf(topic, sizeof(topic), "ferduino/%s/telemetry/temps", deviceId);

  char msg[192];
  snprintf(msg, sizeof(msg),
           "{"
             "\"water_x10\":%d,"
             "\"air_x10\":%d,"
             "\"water_valid\":%u,"
             "\"air_valid\":%u,"
             "\"ts_ms\":%lu"
           "}",
           (int)toX10(t.water_c),
           (int)toX10(t.air_c),
           (unsigned)(t.water_valid ? 1 : 0),
           (unsigned)(t.air_valid ? 1 : 0),
           (unsigned long)t.ts_ms);

  (void)hal::mqtt().publish(topic, (const uint8_t*)msg, strlen(msg), false);
}

} // namespace app::runtime
