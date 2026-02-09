#include "app/scheduler/app_scheduler.h"

#include <Arduino.h>

#ifndef APP_SCHEDULER_USE_RTC
#define APP_SCHEDULER_USE_RTC 0
#endif

namespace app::scheduler {

// Hook débil: cuando haya RTC real, implementa esta función en cualquier .cpp:
//
// extern "C" uint16_t app_scheduler_rtc_minute_of_day(void) {
//   // devolver 0..1439 según hora local
// }
//
// Si no está implementada, el linker usará esta débil (0xFFFF) y haremos fallback a millis.
extern "C" uint16_t app_scheduler_rtc_minute_of_day(void) __attribute__((weak));
extern "C" uint16_t app_scheduler_rtc_minute_of_day(void) { return 0xFFFF; }

static uint16_t s_lastMinuteOfDay = 0;
static bool s_minuteTick = false;
static TimeSource s_src = TimeSource::Millis;

static uint32_t calcMinutesSinceBoot() {
  return (uint32_t)(millis() / 60000UL);
}

static uint16_t minuteOfDayFromMillis() {
  const uint32_t m = calcMinutesSinceBoot();
  return (uint16_t)(m % 1440UL);
}

static uint16_t minuteOfDayFromRtcOrFallback() {
#if APP_SCHEDULER_USE_RTC
  const uint16_t v = app_scheduler_rtc_minute_of_day();
  if (v != 0xFFFF && v < 1440) {
    s_src = TimeSource::Rtc;
    return v;
  }
#endif
  s_src = TimeSource::Millis;
  return minuteOfDayFromMillis();
}

void begin() {
  s_lastMinuteOfDay = minuteOfDayFromRtcOrFallback();
  s_minuteTick = false;
}

void loop() {
  const uint16_t mod = minuteOfDayFromRtcOrFallback();
  if (mod != s_lastMinuteOfDay) {
    s_lastMinuteOfDay = mod;
    s_minuteTick = true;
  }
}

uint32_t minutesSinceBoot() {
  return calcMinutesSinceBoot();
}

uint16_t minuteOfDay() {
  return minuteOfDayFromRtcOrFallback();
}

TimeHM now() {
  const uint16_t mod = minuteOfDay();
  TimeHM t;
  t.hour = (uint8_t)(mod / 60);
  t.minute = (uint8_t)(mod % 60);
  return t;
}

bool minuteTick() {
  return s_minuteTick;
}

bool consumeMinuteTick() {
  const bool v = s_minuteTick;
  s_minuteTick = false;
  return v;
}

TimeSource timeSource() {
  // El source se determina en la última llamada a minuteOfDayFromRtcOrFallback()
  return s_src;
}

} // namespace app::scheduler
