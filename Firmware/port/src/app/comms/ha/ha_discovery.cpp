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

    // Outlets como switch (B2.3)
  // command_topic: ferduino/<id>/cmd/outlet/N (payload "1"/"0")
  // state: value_json.outlet_N

  for (int n = 1; n <= 9; n++) {
    char objectId[16];
    char name[20];
    char uniq[16];
    char topic[190];

    snprintf(objectId, sizeof(objectId), "outlet_%d", n);
    snprintf(name, sizeof(name), "Outlet %d", n);
    snprintf(uniq, sizeof(uniq), "outlet_%d", n);

    snprintf(topic, sizeof(topic),
             "%s/switch/ferduino_%s/%s/config",
             DISCOVERY_PREFIX, FERDUINO_DEVICE_ID, objectId);

    char stateTopic[96], availTopic[96];
    buildTopics(stateTopic, sizeof(stateTopic), availTopic, sizeof(availTopic));

    char cmdTopic[96];
    snprintf(cmdTopic, sizeof(cmdTopic), "%s/%s/cmd/outlet/%d", BASE_TOPIC, FERDUINO_DEVICE_ID, n);

    char device[220];
    buildDeviceJson(device, sizeof(device));

    char payload[700];
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
             FERDUINO_DEVICE_ID, uniq,
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
