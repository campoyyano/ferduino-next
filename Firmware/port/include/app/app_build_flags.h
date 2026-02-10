#pragma once

// ============================================================
// app_build_flags.h
// Centraliza los flags de compilación para selección de backends
// (FAKE vs REAL) y evita #ifndef dispersos por los módulos.
//
// Política:
// - 0 => backend FAKE / stub (sin hardware)
// - 1 => backend REAL (hardware), detrás de HAL o hooks estrechos
// ============================================================

// ============================================================
// Flags de habilitación (enable)
// Permiten aislar módulos para debug/testing sin tocar código.
// ============================================================

#ifndef APP_ENABLE_SCHEDULER
#define APP_ENABLE_SCHEDULER 1
#endif

#ifndef APP_ENABLE_EVENTS_SCHEDULER
#define APP_ENABLE_EVENTS_SCHEDULER 1
#endif

#ifndef APP_ENABLE_OUTLETS
#define APP_ENABLE_OUTLETS 1
#endif

#ifndef APP_ENABLE_SENSORS
#define APP_ENABLE_SENSORS 1
#endif

#ifndef APP_ENABLE_TEMPCTRL
#define APP_ENABLE_TEMPCTRL 1
#endif

#ifndef APP_ENABLE_TELEMETRY
#define APP_ENABLE_TELEMETRY 1
#endif

#ifndef APP_ENABLE_TELEMETRY_TEMPS
#define APP_ENABLE_TELEMETRY_TEMPS 1
#endif

#ifndef APP_ENABLE_TELEMETRY_TEMPCTRL
#define APP_ENABLE_TELEMETRY_TEMPCTRL 1
#endif

// Scheduler: 0 = millis() FAKE, 1 = RTC real (requiere hook app_scheduler_rtc_minute_of_day)
#ifndef APP_SCHEDULER_USE_RTC
#define APP_SCHEDULER_USE_RTC 0
#endif

// Outlets: 0 = stub (sin relés), 1 = HAL relés
#ifndef APP_OUTLETS_USE_RELAY_HAL
#define APP_OUTLETS_USE_RELAY_HAL 0
#endif

// Sensors: 0 = FAKE (sin hardware), 1 = HW (cuando existan drivers/HAL)
#ifndef APP_SENSORS_USE_HW
#define APP_SENSORS_USE_HW 0
#endif

// Control de temperatura (heater/chiller pines directos Mega):
// 0 = solo cálculo (sin escribir GPIO), 1 = activa salidas reales vía hal_gpio.
#ifndef APP_TEMPCTRL_USE_GPIO
#define APP_TEMPCTRL_USE_GPIO 0
#endif
