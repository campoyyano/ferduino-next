#include "app/runtime/app_tempctrl_telemetry.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#include "app/app_build_flags.h"
#include "app/config/app_config.h"
#include "app/temp_control/temp_control.h"
#include "hal/hal_mqtt.h"

namespace app::runtime {

static uint32_t g_lastMs = 0;

void tempctrlTelemetryLoop(uint32_t everySeconds) {
#if !APP_ENABLE_TELEMETRY_TEMPCTRL
  (void)everySeconds;
  return;
#endif
  if (everySeconds == 0) return;
  if (!hal::mqtt().connected()) return;

  const uint32_t now = millis();
  const uint32_t periodMs = everySeconds * 1000UL;
  if ((now - g_lastMs) < periodMs) return;
  g_lastMs = now;

  const auto st = app::tempctrl::last();

  const auto& cfg = app::cfg::get();
  const char* deviceId = cfg.mqtt.deviceId;

  char topic[128];
  snprintf(topic, sizeof(topic), "ferduino/%s/telemetry/tempctrl", deviceId);

  // JSON compacto (sin floats, x10)
  char payload[220];
  snprintf(payload, sizeof(payload),
           "{"
             "\"valid\":%u,"
             "\"water_x10\":%d,"
             "\"set_x10\":%d,"
             "\"off_x10\":%d,"
             "\"alarm_x10\":%d,"
             "\"heater_on\":%u,"
             "\"chiller_on\":%u,"
             "\"alarm_active\":%u"
           "}",
           (unsigned)(st.valid ? 1 : 0),
           (int)st.water_x10,
           (int)st.cfg.set_x10,
           (int)st.cfg.off_x10,
           (int)st.cfg.alarm_x10,
           (unsigned)(st.heater_on ? 1 : 0),
           (unsigned)(st.chiller_on ? 1 : 0),
           (unsigned)(st.alarm_active ? 1 : 0));

  (void)hal::mqtt().publish(topic, (const uint8_t*)payload, strlen(payload), false);
}

} // namespace app::runtime
