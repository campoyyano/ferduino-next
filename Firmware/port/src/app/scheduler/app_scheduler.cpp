#include "app/scheduler/app_scheduler.h"

#include <Arduino.h>

#include "app/app_build_flags.h"

namespace app::scheduler {

// Hook débil: cuando haya RTC real, implementaremos esto en el backend RTC/HAL.
// Debe devolver minutos desde medianoche (0..1439), o 0xFFFF si no disponible.
extern "C" uint16_t __attribute__((weak)) app_scheduler_rtc_minute_of_day(void) {
  return 0xFFFF;
}

static uint32_t g_bootMs = 0;
static uint32_t g_lastPollMs = 0;
static uint16_t g_minuteOfDay = 0;
static uint32_t g_minutesSinceBoot = 0;
static bool g_usingRtc = false;

static bool g_minuteTick = false;
static uint16_t g_lastMinuteOfDay = 0xFFFF;

static uint16_t clampMinute(uint16_t m) {
  return (m >= 1440) ? (uint16_t)(m % 1440) : m;
}

static uint16_t minuteFromMillis(uint32_t nowMs) {
  const uint32_t elapsed = nowMs - g_bootMs;
  const uint32_t minutes = elapsed / 60000UL;
  return clampMinute((uint16_t)(minutes % 1440UL));
}

static uint32_t minutesSinceBootFromMillis(uint32_t nowMs) {
  const uint32_t elapsed = nowMs - g_bootMs;
  return elapsed / 60000UL;
}

static void updateMinuteTick(uint16_t newMinuteOfDay) {
  if (g_lastMinuteOfDay == 0xFFFF) {
    g_lastMinuteOfDay = newMinuteOfDay;
    g_minuteTick = true; // primer valor disponible
    return;
  }
  if (newMinuteOfDay != g_lastMinuteOfDay) {
    g_minuteTick = true;
    g_lastMinuteOfDay = newMinuteOfDay;
  }
}

void begin() {
  const uint32_t now = millis();
  g_bootMs = now;
  g_lastPollMs = now;

  g_minuteTick = false;
  g_lastMinuteOfDay = 0xFFFF;

#if APP_SCHEDULER_USE_RTC
  const uint16_t rtcMin = app_scheduler_rtc_minute_of_day();
  if (rtcMin != 0xFFFF) {
    g_usingRtc = true;
    g_minuteOfDay = clampMinute(rtcMin);
    g_minutesSinceBoot = minutesSinceBootFromMillis(now);
    updateMinuteTick(g_minuteOfDay);
    return;
  }
#endif

  g_usingRtc = false;
  g_minuteOfDay = minuteFromMillis(now);
  g_minutesSinceBoot = minutesSinceBootFromMillis(now);
  updateMinuteTick(g_minuteOfDay);
}

void loop() {
  const uint32_t now = millis();

  // Poll cada ~250ms para detectar cambios de minuto con bajo coste.
  if ((uint32_t)(now - g_lastPollMs) < 250UL) return;
  g_lastPollMs = now;

#if APP_SCHEDULER_USE_RTC
  const uint16_t rtcMin = app_scheduler_rtc_minute_of_day();
  if (rtcMin != 0xFFFF) {
    g_usingRtc = true;
    g_minuteOfDay = clampMinute(rtcMin);
    g_minutesSinceBoot = minutesSinceBootFromMillis(now);
    updateMinuteTick(g_minuteOfDay);
    return;
  }
#endif

  g_usingRtc = false;
  g_minuteOfDay = minuteFromMillis(now);
  g_minutesSinceBoot = minutesSinceBootFromMillis(now);
  updateMinuteTick(g_minuteOfDay);
}

TimeHM now() {
  TimeHM t{};
  t.hour = (uint8_t)(g_minuteOfDay / 60);
  t.minute = (uint8_t)(g_minuteOfDay % 60);
  return t;
}

uint16_t minuteOfDay() {
  return g_minuteOfDay;
}

uint32_t minutesSinceBoot() {
  return g_minutesSinceBoot;
}

bool minuteTick() {
  return g_minuteTick;
}

bool consumeMinuteTick() {
  const bool v = g_minuteTick;
  g_minuteTick = false;
  return v;
}

TimeSource timeSource() {
  return g_usingRtc ? TimeSource::Rtc : TimeSource::Millis;
}

} // namespace app::scheduler
