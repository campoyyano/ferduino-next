#include "app/comms/ha/ha_discovery.h"

#include "hal/hal_mqtt.h"

#include <Arduino.h>
#include <string.h>
#include <stdio.h>

#ifndef FERDUINO_DEVICE_ID
  #error "FERDUINO_DEVICE_ID must be defined in platformio.ini build_flags"
#endif

namespace app::ha {

static constexpr const char* DISCOVERY_PREFIX = "homeassistant";
static constexpr const char* BASE_TOPIC       = "ferduino";

static void publishRetained(const char* topic, const char* payload) {
  if (!topic || !payload) return;
  (void)hal::mqtt().publish(topic, (const uint8_t*)payload, strlen(payload), true);
}

static void buildDeviceJson(char* out, size_t outLen) {
  snprintf(out, outLen,
           "{"
             "\"identifiers\":[\"%s\"],"
             "\"manufacturer\":\"Ferduino\","
             "\"model\":\"Ferduino Next (Mega2560)\","
             "\"name\":\"Ferduino Next\","
             "\"sw_version\":\"ferduino-next\""
           "}",
           FERDUINO_DEVICE_ID);
}

static void buildTopics(char* stateTopic, size_t stLen,
                        char* availTopic, size_t avLen) {
  snprintf(stateTopic, stLen, "%s/%s/state", BASE_TOPIC, FERDUINO_DEVICE_ID);
  snprintf(availTopic, avLen, "%s/%s/availability", BASE_TOPIC, FERDUINO_DEVICE_ID);
}

static void publishSensor(const char* objectId,
                          const char* name,
                          const char* uniqueSuffix,
                          const char* devClass,   // puede ser "" si no aplica
                          const char* unit,       // puede ser "" si no aplica
                          const char* valueTemplate) {
  char topic[180];
  snprintf(topic, sizeof(topic),
           "%s/sensor/ferduino_%s/%s/config",
           DISCOVERY_PREFIX, FERDUINO_DEVICE_ID, objectId);

  char stateTopic[96], availTopic[96];
  buildTopics(stateTopic, sizeof(stateTopic), availTopic, sizeof(availTopic));

  char device[220];
  buildDeviceJson(device, sizeof(device));

  char payload[640];

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
             FERDUINO_DEVICE_ID, uniqueSuffix,
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
             FERDUINO_DEVICE_ID, uniqueSuffix,
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
             FERDUINO_DEVICE_ID, uniqueSuffix,
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
             FERDUINO_DEVICE_ID, uniqueSuffix,
             stateTopic,
             availTopic,
             valueTemplate,
             device);
  }

  publishRetained(topic, payload);
}

static void publishBinarySensor(const char* objectId,
                                const char* name,
                                const char* uniqueSuffix,
                                const char* valueTemplate) {
  char topic[190];
  snprintf(topic, sizeof(topic),
           "%s/binary_sensor/ferduino_%s/%s/config",
           DISCOVERY_PREFIX, FERDUINO_DEVICE_ID, objectId);

  char stateTopic[96], availTopic[96];
  buildTopics(stateTopic, sizeof(stateTopic), availTopic, sizeof(availTopic));

  char device[220];
  buildDeviceJson(device, sizeof(device));

  char payload[560];
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
           FERDUINO_DEVICE_ID, uniqueSuffix,
           stateTopic,
           availTopic,
           valueTemplate,
           device);

  publishRetained(topic, payload);
}

static void publishAllEntities() {
  // Temperaturas
  publishSensor("temp_water",    "Water Temp",    "temp_water",    "temperature", "°C", "{{ value_json.water_temperature }}");
  publishSensor("temp_heatsink", "Heatsink Temp", "temp_heatsink", "temperature", "°C", "{{ value_json.heatsink_temperature }}");
  publishSensor("temp_ambient",  "Ambient Temp",  "temp_ambient",  "temperature", "°C", "{{ value_json.ambient_temperature }}");

  // Química
  publishSensor("water_ph",   "Water pH",   "water_ph",   "", "", "{{ value_json.water_ph }}");
  publishSensor("reactor_ph", "Reactor pH", "reactor_ph", "", "", "{{ value_json.reactor_ph }}");
  publishSensor("orp",        "ORP",        "orp",        "", "mV", "{{ value_json.orp }}");
  publishSensor("salinity",   "Salinity",   "salinity",   "", "", "{{ value_json.salinity }}");

  // LEDs
  publishSensor("led_white",      "LED White",      "led_white",      "", "", "{{ value_json.led_white_power }}");
  publishSensor("led_blue",       "LED Blue",       "led_blue",       "", "", "{{ value_json.led_blue_power }}");
  publishSensor("led_royal_blue", "LED Royal Blue", "led_royal_blue", "", "", "{{ value_json.led_royal_blue_power }}");
  publishSensor("led_red",        "LED Red",        "led_red",        "", "", "{{ value_json.led_red_power }}");
  publishSensor("led_uv",         "LED UV",         "led_uv",         "", "", "{{ value_json.led_uv_power }}");

  // Uptime
  publishSensor("uptime", "Uptime", "uptime", "", "s", "{{ value_json.uptime }}");

  // Outlets read-only (B2.2) como binary_sensor
  publishBinarySensor("outlet_1", "Outlet 1", "outlet_1", "{{ 'ON' if value_json.outlet_1 == 1 else 'OFF' }}");
  publishBinarySensor("outlet_2", "Outlet 2", "outlet_2", "{{ 'ON' if value_json.outlet_2 == 1 else 'OFF' }}");
  publishBinarySensor("outlet_3", "Outlet 3", "outlet_3", "{{ 'ON' if value_json.outlet_3 == 1 else 'OFF' }}");
  publishBinarySensor("outlet_4", "Outlet 4", "outlet_4", "{{ 'ON' if value_json.outlet_4 == 1 else 'OFF' }}");
  publishBinarySensor("outlet_5", "Outlet 5", "outlet_5", "{{ 'ON' if value_json.outlet_5 == 1 else 'OFF' }}");
  publishBinarySensor("outlet_6", "Outlet 6", "outlet_6", "{{ 'ON' if value_json.outlet_6 == 1 else 'OFF' }}");
  publishBinarySensor("outlet_7", "Outlet 7", "outlet_7", "{{ 'ON' if value_json.outlet_7 == 1 else 'OFF' }}");
  publishBinarySensor("outlet_8", "Outlet 8", "outlet_8", "{{ 'ON' if value_json.outlet_8 == 1 else 'OFF' }}");
  publishBinarySensor("outlet_9", "Outlet 9", "outlet_9", "{{ 'ON' if value_json.outlet_9 == 1 else 'OFF' }}");
}

void publishDiscoveryMinimal() {
  publishAllEntities();
}

void publishDiscoveryAll() {
  publishAllEntities();
}

} // namespace app::ha
