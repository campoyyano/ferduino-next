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
static uint32_t g_lastTickMs = 0;
static uint16_t g_minuteOfDay = 0;
static bool g_usingRtc = false;

static uint16_t clampMinute(uint16_t m) {
  return (m >= 1440) ? (uint16_t)(m % 1440) : m;
}

static uint16_t minuteFromMillis(uint32_t nowMs) {
  // minuto desde "boot" en base a millis()
  const uint32_t elapsed = nowMs - g_bootMs;
  const uint32_t minutes = elapsed / 60000UL;
  return clampMinute((uint16_t)(minutes % 1440UL));
}

void begin() {
  g_bootMs = millis();
  g_lastTickMs = g_bootMs;

#if APP_SCHEDULER_USE_RTC
  const uint16_t rtcMin = app_scheduler_rtc_minute_of_day();
  if (rtcMin != 0xFFFF) {
    g_usingRtc = true;
    g_minuteOfDay = clampMinute(rtcMin);
    return;
  }
#endif

  g_usingRtc = false;
  g_minuteOfDay = minuteFromMillis(g_bootMs);
}

void loop() {
  const uint32_t now = millis();

#if APP_SCHEDULER_USE_RTC
  const uint16_t rtcMin = app_scheduler_rtc_minute_of_day();
  if (rtcMin != 0xFFFF) {
    g_usingRtc = true;
    g_minuteOfDay = clampMinute(rtcMin);
    g_lastTickMs = now;
    return;
  }
#endif

  // fallback a millis()
  g_usingRtc = false;

  // tick cada ~1s para no recalcular constantemente
  if ((uint32_t)(now - g_lastTickMs) < 1000UL) return;
  g_lastTickMs = now;

  g_minuteOfDay = minuteFromMillis(now);
}

uint16_t minuteOfDay() {
  return g_minuteOfDay;
}

uint8_t hour() {
  return (uint8_t)(g_minuteOfDay / 60);
}

uint8_t minute() {
  return (uint8_t)(g_minuteOfDay % 60);
}

bool usingRtc() {
  return g_usingRtc;
}

uint32_t tickMs() {
  // "tick" para telemetría/debug: marca tiempo desde boot
  return millis() - g_bootMs;
}

} // namespace app::scheduler
