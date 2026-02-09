#include "app/comms/ha/ha_discovery.h"

#include "app/config/app_config.h"
#include "hal/hal_mqtt.h"

#include <Arduino.h>
#include <string.h>
#include <stdio.h>

namespace app::ha {

static constexpr const char* DISCOVERY_PREFIX = "homeassistant";
static constexpr const char* BASE_TOPIC       = "ferduino";

static void publishRetained(const char* topic, const char* payload) {
  if (!topic || !payload) return;
  (void)hal::mqtt().publish(topic, (const uint8_t*)payload, strlen(payload), true);
}

static void buildDeviceJson(const char* deviceId, char* out, size_t outLen) {
  snprintf(out, outLen,
           "{"
             "\"identifiers\":[\"%s\"],"
             "\"manufacturer\":\"Ferduino\","
             "\"model\":\"Ferduino Next (Mega2560)\","
             "\"name\":\"Ferduino Next\","
             "\"sw_version\":\"ferduino-next\""
           "}",
           deviceId ? deviceId : "");
}

static void buildTopics(const char* deviceId,
                        char* stateTopic, size_t stLen,
                        char* availTopic, size_t avLen) {
  snprintf(stateTopic, stLen, "%s/%s/state", BASE_TOPIC, deviceId ? deviceId : "");
  snprintf(availTopic, avLen, "%s/%s/availability", BASE_TOPIC, deviceId ? deviceId : "");
}

static void publishSensor(const char* deviceId,
                          const char* objectId,
                          const char* name,
                          const char* uniqueSuffix,
                          const char* devClass,       // "" si no aplica
                          const char* unit,           // "" si no aplica
                          const char* valueTemplate) {
  char topic[200];
  snprintf(topic, sizeof(topic),
           "%s/sensor/ferduino_%s/%s/config",
           DISCOVERY_PREFIX, deviceId, objectId);

  char stateTopic[96], availTopic[96];
  buildTopics(deviceId, stateTopic, sizeof(stateTopic), availTopic, sizeof(availTopic));

  char device[220];
  buildDeviceJson(deviceId, device, sizeof(device));

  char payload[700];

  if (devClass && devClass[0] && unit && unit[0]) {
    snprintf(payload, sizeof(payload),
             "{"
               "\"name\":\"%s\","
               "\"uniq_id\":\"ferduino_%s_%s\","
               "\"stat_t\":\"%s\","
               "\"avty_t\":\"%s\","
               "\"pl_avail\":\"online\","
               "\"pl_not_avail\":\"offline\","
               "\"dev_cla\":\"%s\","
               "\"unit_of_meas\":\"%s\","
               "\"val_tpl\":\"%s\","
               "\"dev\":%s"
             "}",
             name,
             deviceId, uniqueSuffix,
             stateTopic,
             availTopic,
             devClass,
             unit,
             valueTemplate,
             device);
  } else if (devClass && devClass[0]) {
    snprintf(payload, sizeof(payload),
             "{"
               "\"name\":\"%s\","
               "\"uniq_id\":\"ferduino_%s_%s\","
               "\"stat_t\":\"%s\","
               "\"avty_t\":\"%s\","
               "\"pl_avail\":\"online\","
               "\"pl_not_avail\":\"offline\","
               "\"dev_cla\":\"%s\","
               "\"val_tpl\":\"%s\","
               "\"dev\":%s"
             "}",
             name,
             deviceId, uniqueSuffix,
             stateTopic,
             availTopic,
             devClass,
             valueTemplate,
             device);
  } else if (unit && unit[0]) {
    snprintf(payload, sizeof(payload),
             "{"
               "\"name\":\"%s\","
               "\"uniq_id\":\"ferduino_%s_%s\","
               "\"stat_t\":\"%s\","
               "\"avty_t\":\"%s\","
               "\"pl_avail\":\"online\","
               "\"pl_not_avail\":\"offline\","
               "\"unit_of_meas\":\"%s\","
               "\"val_tpl\":\"%s\","
               "\"dev\":%s"
             "}",
             name,
             deviceId, uniqueSuffix,
             stateTopic,
             availTopic,
             unit,
             valueTemplate,
             device);
  } else {
    snprintf(payload, sizeof(payload),
             "{"
               "\"name\":\"%s\","
               "\"uniq_id\":\"ferduino_%s_%s\","
               "\"stat_t\":\"%s\","
               "\"avty_t\":\"%s\","
               "\"pl_avail\":\"online\","
               "\"pl_not_avail\":\"offline\","
               "\"val_tpl\":\"%s\","
               "\"dev\":%s"
             "}",
             name,
             deviceId, uniqueSuffix,
             stateTopic,
             availTopic,
             valueTemplate,
             device);
  }

  publishRetained(topic, payload);
}

static void publishAllEntities() {
  // Cinturón y tirantes: si todavía no se cargó cfg (no hay setup() global),
  // garantizamos defaults.
  if (app::cfg::get().mqtt.deviceId[0] == '\0') {
    (void)app::cfg::loadOrDefault();
  }

  const auto& cfg = app::cfg::get();
  const char* deviceId = cfg.mqtt.deviceId;

  // Temperaturas
  publishSensor(deviceId, "temp_water",    "Water Temp",    "temp_water",    "temperature", "°C", "{{ value_json.water_temperature }}");
  publishSensor(deviceId, "temp_heatsink", "Heatsink Temp", "temp_heatsink", "temperature", "°C", "{{ value_json.heatsink_temperature }}");
  publishSensor(deviceId, "temp_ambient",  "Ambient Temp",  "temp_ambient",  "temperature", "°C", "{{ value_json.ambient_temperature }}");

  // Química
  publishSensor(deviceId, "water_ph",   "Water pH",   "water_ph",   "", "", "{{ value_json.water_ph }}");
  publishSensor(deviceId, "reactor_ph", "Reactor pH", "reactor_ph", "", "", "{{ value_json.reactor_ph }}");
  publishSensor(deviceId, "orp",        "ORP",        "orp",        "", "mV", "{{ value_json.orp }}");
  publishSensor(deviceId, "salinity",   "Salinity",   "salinity",   "", "", "{{ value_json.salinity }}");

  // LEDs
  publishSensor(deviceId, "led_white",      "LED White",      "led_white",      "", "", "{{ value_json.led_white_power }}");
  publishSensor(deviceId, "led_blue",       "LED Blue",       "led_blue",       "", "", "{{ value_json.led_blue_power }}");
  publishSensor(deviceId, "led_royal_blue", "LED Royal Blue", "led_royal_blue", "", "", "{{ value_json.led_royal_blue_power }}");
  publishSensor(deviceId, "led_red",        "LED Red",        "led_red",        "", "", "{{ value_json.led_red_power }}");
  publishSensor(deviceId, "led_uv",         "LED UV",         "led_uv",         "", "", "{{ value_json.led_uv_power }}");

  // Uptime
  publishSensor(deviceId, "uptime", "Uptime", "uptime", "", "s", "{{ value_json.uptime }}");

  // Scheduler / Clock (B6.2a)
  publishSensor(deviceId, "clock_minute_of_day", "Clock Minute Of Day", "clock_minute_of_day", "", "min", "{{ value_json.clock_minute_of_day }}");
  publishSensor(deviceId, "clock_source", "Clock Source", "clock_source", "", "", "{{ value_json.clock_source }}");
  publishSensor(deviceId, "clock_tick", "Clock Minute Tick", "clock_tick", "", "", "{{ value_json.clock_tick }}");

  // Outlets como switch (B2.3)
  // command_topic: ferduino/<id>/cmd/outlet/N (payload "1"/"0")
  // state: value_json.outlet_N
  for (int n = 1; n <= 9; n++) {
    char objectId[16];
    char name[20];
    char uniq[16];
    char topic[220];

    snprintf(objectId, sizeof(objectId), "outlet_%d", n);
    snprintf(name, sizeof(name), "Outlet %d", n);
    snprintf(uniq, sizeof(uniq), "outlet_%d", n);

    snprintf(topic, sizeof(topic),
             "%s/switch/ferduino_%s/%s/config",
             DISCOVERY_PREFIX, deviceId, objectId);

    char stateTopic[96], availTopic[96];
    buildTopics(deviceId, stateTopic, sizeof(stateTopic), availTopic, sizeof(availTopic));

    char cmdTopic[96];
    snprintf(cmdTopic, sizeof(cmdTopic), "%s/%s/cmd/outlet/%d", BASE_TOPIC, deviceId, n);

    char device[220];
    buildDeviceJson(deviceId, device, sizeof(device));

    char payload[720];
    snprintf(payload, sizeof(payload),
             "{"
               "\"name\":\"%s\","
               "\"uniq_id\":\"ferduino_%s_%s\","
               "\"stat_t\":\"%s\","
               "\"cmd_t\":\"%s\","
               "\"avty_t\":\"%s\","
               "\"pl_avail\":\"online\","
               "\"pl_not_avail\":\"offline\","
               "\"pl_on\":\"1\","
               "\"pl_off\":\"0\","
               "\"val_tpl\":\"{{ value_json.%s }}\","
               "\"dev\":%s"
             "}",
             name,
             deviceId, uniq,
             stateTopic,
             cmdTopic,
             availTopic,
             objectId,
             device);

    publishRetained(topic, payload);
  }
}

void publishDiscoveryMinimal() {
  publishAllEntities();
}

void publishDiscoveryAll() {
  publishAllEntities();
}

} // namespace app::ha
