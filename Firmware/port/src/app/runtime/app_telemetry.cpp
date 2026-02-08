#include "app/runtime/app_telemetry.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#include "app/config/app_config.h"
#include "hal/hal_mqtt.h"

namespace app::runtime {

static uint32_t g_lastMs = 0;

void telemetryLoop(uint32_t intervalSec) {
  if (!hal::mqtt().connected()) return;

  const auto& cfg = app::cfg::get();
  const char* deviceId = cfg.mqtt.deviceId;
  if (!deviceId || deviceId[0] == '\0') return;

  const uint32_t now = millis();
  const uint32_t intervalMs = intervalSec * 1000UL;

  if (g_lastMs != 0 && (uint32_t)(now - g_lastMs) < intervalMs) return;
  g_lastMs = now;

  char topic[128];
  snprintf(topic, sizeof(topic), "ferduino/%s/telemetry/uptime", deviceId);

  char msg[48];
  snprintf(msg, sizeof(msg), "{\"ms\":%lu}", (unsigned long)now);

  (void)hal::mqtt().publish(topic, (const uint8_t*)msg, strlen(msg), false);
}

} // namespace app::runtime
