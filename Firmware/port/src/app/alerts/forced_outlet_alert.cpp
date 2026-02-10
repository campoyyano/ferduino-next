#include "app/alerts/forced_outlet_alert.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#include "app/config/app_config.h"
#include "app/nvm/eeprom_registry.h"
#include "app/outlets/app_outlets.h"
#include "app/scheduler/app_scheduler.h"

#include "hal/hal_mqtt.h"

namespace app::alerts::forced_outlets {

static constexpr uint16_t kForcedAlertMinuteOfDayKey = 360; // u32 minute-of-day
static constexpr uint16_t kForcedAlertEnabledKey     = 361; // bool

static constexpr uint16_t kDefaultMinuteOfDay = 9u * 60u; // 09:00

static bool g_started = false;

static uint16_t g_cfgMinuteOfDay = kDefaultMinuteOfDay;
static bool g_cfgEnabled = true;

static uint16_t g_prevForcedMask = 0;

static uint16_t g_lastMinuteSeen = 0xFFFF;
static uint32_t g_dayCounter = 0;
static uint32_t g_lastDaySent = 0xFFFFFFFFu;

static uint16_t clampMinuteOfDay(uint32_t v) {
  if (v > 1439u) return 1439u;
  return (uint16_t)v;
}

static uint16_t computeForcedMask() {
  uint16_t mask = 0;
  for (uint8_t i = 0; i < app::outlets::count(); ++i) {
    if (!app::outlets::isAuto(i)) {
      mask |= (uint16_t)(1u << i);
    }
  }
  return mask;
}

static void publishForcedOutlets(const char* reason, uint16_t mask) {
  if (!hal::mqtt().connected()) return;

  const char* deviceId = app::cfg::get().mqtt.deviceId;

  char topic[96];
  snprintf(topic, sizeof(topic), "ferduino/%s/alert/forced_outlets", deviceId);

  char payload[168];
  snprintf(payload, sizeof(payload),
           "{\"active\":%s,\"mask\":%u,\"reason\":\"%s\",\"minOfDay\":%u}",
           (mask != 0) ? "true" : "false",
           (unsigned)mask,
           reason ? reason : "unknown",
           (unsigned)app::scheduler::minuteOfDay());

  (void)hal::mqtt().publish(topic, (const uint8_t*)payload, strlen(payload), false);
}

static void loadCfg() {
  auto& reg = app::nvm::registry();

  uint32_t v = 0;
  if (reg.getU32(kForcedAlertMinuteOfDayKey, v)) {
    g_cfgMinuteOfDay = clampMinuteOfDay(v);
  } else {
    g_cfgMinuteOfDay = kDefaultMinuteOfDay;
    (void)reg.setU32(kForcedAlertMinuteOfDayKey, (uint32_t)g_cfgMinuteOfDay);
  }

  bool en = true;
  if (reg.getBool(kForcedAlertEnabledKey, en)) {
    g_cfgEnabled = en;
  } else {
    g_cfgEnabled = true;
    (void)reg.setBool(kForcedAlertEnabledKey, true);
  }

  (void)reg.commit();
}

void begin() {
  if (g_started) return;
  g_started = true;

  loadCfg();

  g_prevForcedMask = g_cfgEnabled ? computeForcedMask() : 0;
  g_lastMinuteSeen = 0xFFFF;
  g_dayCounter = 0;
  g_lastDaySent = 0xFFFFFFFFu;
}

void loop() {
  if (!g_started) begin();

  const uint16_t nowMin = app::scheduler::minuteOfDay();

  // day counter based on wrap-around of minute-of-day
  if (g_lastMinuteSeen != 0xFFFF) {
    if (nowMin < g_lastMinuteSeen) {
      g_dayCounter++;
    }
  }
  g_lastMinuteSeen = nowMin;

  if (!hal::mqtt().connected()) return;

  if (!g_cfgEnabled) {
    if (g_prevForcedMask != 0) {
      publishForcedOutlets("disabled", 0);
      g_prevForcedMask = 0;
    }
    return;
  }

  const uint16_t forcedMask = computeForcedMask();

  // immediate publish on change
  if (forcedMask != g_prevForcedMask) {
    g_prevForcedMask = forcedMask;
    publishForcedOutlets("change", forcedMask);
  }

  // daily reminder at configured minute
  if (nowMin == g_cfgMinuteOfDay) {
    if (forcedMask != 0 && g_dayCounter != g_lastDaySent) {
      g_lastDaySent = g_dayCounter;
      publishForcedOutlets("daily", forcedMask);
    } else if (forcedMask == 0) {
      g_lastDaySent = g_dayCounter;
    }
  }
}

app::scheduler::TimeHM reminderTime() {
  app::scheduler::TimeHM t{0, 0};
  const uint16_t v = g_cfgMinuteOfDay;
  t.hour = (uint8_t)(v / 60u);
  t.minute = (uint8_t)(v % 60u);
  return t;
}

void setReminderTime(const app::scheduler::TimeHM& t) {
  const uint16_t v = (uint16_t)((uint16_t)t.hour * 60u + (uint16_t)t.minute);
  g_cfgMinuteOfDay = clampMinuteOfDay(v);

  auto& reg = app::nvm::registry();
  (void)reg.setU32(kForcedAlertMinuteOfDayKey, (uint32_t)g_cfgMinuteOfDay);
  (void)reg.commit();
}

bool enabled() {
  return g_cfgEnabled;
}

void setEnabled(bool en) {
  g_cfgEnabled = en;

  auto& reg = app::nvm::registry();
  (void)reg.setBool(kForcedAlertEnabledKey, en);
  (void)reg.commit();

  // reset internal counters on disable
  if (!en) {
    g_lastDaySent = 0xFFFFFFFFu;
    g_prevForcedMask = 0;
  } else {
    g_prevForcedMask = computeForcedMask();
  }
}

} // namespace app::alerts::forced_outlets
