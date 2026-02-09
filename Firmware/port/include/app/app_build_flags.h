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
