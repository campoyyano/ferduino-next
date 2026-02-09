#include "app/runtime/app_telemetry.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#include "app/config/app_config.h"
#include "app/scheduler/app_scheduler.h"
#include "app/scheduler/app_event_scheduler.h"
#include "hal/hal_mqtt.h"

namespace app::runtime {

static uint32_t g_lastMs = 0;

static const char* sourceToStr(app::scheduler::TimeSource s) {
  return (s == app::scheduler::TimeSource::Rtc) ? "rtc" : "millis";
}

void telemetryLoop(uint32_t intervalSec) {
  if (!hal::mqtt().connected()) return;

  const auto& cfg = app::cfg::get();
  const char* deviceId = cfg.mqtt.deviceId;
  if (!deviceId || deviceId[0] == '\0') return;

  const uint32_t now = millis();
  const uint32_t intervalMs = intervalSec * 1000UL;

  if (g_lastMs != 0 && (uint32_t)(now - g_lastMs) < intervalMs) return;
  g_lastMs = now;

  // --- uptime ---
  {
    char topic[128];
    snprintf(topic, sizeof(topic), "ferduino/%s/telemetry/uptime", deviceId);

    char msg[64];
    snprintf(msg, sizeof(msg), "{\"ms\":%lu}", (unsigned long)now);

    (void)hal::mqtt().publish(topic, (const uint8_t*)msg, strlen(msg), false);
  }

  // --- scheduler debug ---
  {
    const app::scheduler::TimeHM t = app::scheduler::now();
    const uint16_t mod = app::scheduler::minuteOfDay();
    const uint32_t minsBoot = app::scheduler::minutesSinceBoot();
    const uint8_t tick = app::scheduler::minuteTick() ? 1 : 0; // peek (no consume)
    const char* src = sourceToStr(app::scheduler::timeSource());

    char topic[128];
    snprintf(topic, sizeof(topic), "ferduino/%s/telemetry/scheduler", deviceId);

    char msg[160];
    snprintf(msg, sizeof(msg),
             "{"
               "\"ms\":%lu,"
               "\"mins_boot\":%lu,"
               "\"min_of_day\":%u,"
               "\"hour\":%u,"
               "\"minute\":%u,"
               "\"tick\":%u,"
               "\"source\":\"%s\""
             "}",
             (unsigned long)now,
             (unsigned long)minsBoot,
             (unsigned)mod,
             (unsigned)t.hour,
             (unsigned)t.minute,
             (unsigned)tick,
             src);

    (void)hal::mqtt().publish(topic, (const uint8_t*)msg, strlen(msg), false);
  }

  // --- event scheduler debug (C1.1, canal 0) ---
  {
    const uint8_t ch = 0;
    const auto w = app::scheduler::events::window(ch);
    const bool want = app::scheduler::events::desiredOn(ch);

    char topic[128];
    snprintf(topic, sizeof(topic), "ferduino/%s/telemetry/event_scheduler", deviceId);

    char msg[160];
    snprintf(msg, sizeof(msg),
             "{"
               "\"ch\":%u,"
               "\"enabled\":%u,"
               "\"on\":%u,"
               "\"off\":%u,"
               "\"desired\":%u"
             "}",
             (unsigned)ch,
             (unsigned)(w.enabled ? 1 : 0),
             (unsigned)w.onMinute,
             (unsigned)w.offMinute,
             (unsigned)(want ? 1 : 0));

    (void)hal::mqtt().publish(topic, (const uint8_t*)msg, strlen(msg), false);
  }
}

} // namespace app::runtime
