#pragma once

#include "app/scheduler/app_scheduler.h"

namespace app::alerts::forced_outlets {

// Publishes recurrent MQTT alerts while any outlet is in MANUAL (auto=false).
// - Immediate publish on mask change (reason="change").
// - Daily reminder at configured time if mask!=0 (reason="daily").
// - If disabled while active, publishes once active=false (reason="disabled").
//
// Topic: ferduino/<deviceId>/alert/forced_outlets (no retained)
// Payload: {"active":true|false,"mask":<u16>,"reason":"change|daily|disabled","minOfDay":<u16>}

void begin();
void loop();

// Daily reminder time (minute-of-day, persisted in NVM).
app::scheduler::TimeHM reminderTime();
void setReminderTime(const app::scheduler::TimeHM& t);

// Enable/disable the feature (persisted in NVM).
bool enabled();
void setEnabled(bool en);

} // namespace app::alerts::forced_outlets
