#include "app/comms/ha/ha_discovery.h"

#include "hal/hal_mqtt.h"

#include <Arduino.h>
#include <string.h>
#include <stdio.h>

#ifndef FERDUINO_DEVICE_ID
  #error "FERDUINO_DEVICE_ID must be defined in platformio.ini build_flags"
#endif

namespace app::ha {

// Defaults (B1): constantes. En B3 se moverán a EEPROM/config runtime.
static constexpr const char* DISCOVERY_PREFIX = "homeassistant";
static constexpr const char* BASE_TOPIC       = "ferduino";

static void publishRetained(const char* topic, const char* payload) {
  if (!topic || !payload) return;
  (void)hal::mqtt().publish(topic, (const uint8_t*)payload, strlen(payload), true);
}

static void buildDeviceJson(char* out, size_t outLen) {
  // device block reutilizable para todas las entidades HA
  // identifiers: array con 1 elemento estable (device_id)
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

static void publishSensorTwater() {
  // homeassistant/sensor/<node_id>/<object_id>/config
  char topic[160];
  snprintf(topic, sizeof(topic),
           "%s/sensor/ferduino_%s/temp_water/config",
           DISCOVERY_PREFIX, FERDUINO_DEVICE_ID);

  // state topic y availability topic
  char stateTopic[96];
  char availTopic[96];
  snprintf(stateTopic, sizeof(stateTopic), "%s/%s/state", BASE_TOPIC, FERDUINO_DEVICE_ID);
  snprintf(availTopic, sizeof(availTopic), "%s/%s/availability", BASE_TOPIC, FERDUINO_DEVICE_ID);

  char device[220];
  buildDeviceJson(device, sizeof(device));

  // Config minimal + estable
  // value_template usa JSON del state topic: {"twater":...}
  char payload[520];
  snprintf(payload, sizeof(payload),
           "{"
             "\"name\":\"Water Temp\","
             "\"uniq_id\":\"ferduino_%s_temp_water\","
             "\"stat_t\":\"%s\","
             "\"avty_t\":\"%s\","
             "\"pl_avail\":\"online\","
             "\"pl_not_avail\":\"offline\","
             "\"dev_cla\":\"temperature\","
             "\"unit_of_meas\":\"°C\","
             "\"val_tpl\":\"{{ value_json.twater }}\","
             "\"dev\":%s"
           "}",
           FERDUINO_DEVICE_ID,
           stateTopic,
           availTopic,
           device);

  publishRetained(topic, payload);
}

static void publishSwitchOutlet1() {
  // homeassistant/switch/<node_id>/<object_id>/config
  char topic[160];
  snprintf(topic, sizeof(topic),
           "%s/switch/ferduino_%s/outlet1/config",
           DISCOVERY_PREFIX, FERDUINO_DEVICE_ID);

  char stateTopic[96];
  char availTopic[96];
  char cmdTopic[96];
  snprintf(stateTopic, sizeof(stateTopic), "%s/%s/state", BASE_TOPIC, FERDUINO_DEVICE_ID);
  snprintf(availTopic, sizeof(availTopic), "%s/%s/availability", BASE_TOPIC, FERDUINO_DEVICE_ID);
  snprintf(cmdTopic, sizeof(cmdTopic), "%s/%s/cmd/outlet/1", BASE_TOPIC, FERDUINO_DEVICE_ID);

  char device[220];
  buildDeviceJson(device, sizeof(device));

  // Switch que usa:
  // - command_topic: "ferduino/<id>/cmd/outlet/1" con payload "1"/"0"
  // - state_topic: "ferduino/<id>/state" con value_template -> outlet1
  char payload[620];
  snprintf(payload, sizeof(payload),
           "{"
             "\"name\":\"Outlet 1\","
             "\"uniq_id\":\"ferduino_%s_outlet1\","
             "\"cmd_t\":\"%s\","
             "\"stat_t\":\"%s\","
             "\"avty_t\":\"%s\","
             "\"pl_avail\":\"online\","
             "\"pl_not_avail\":\"offline\","
             "\"pl_on\":\"1\","
             "\"pl_off\":\"0\","
             "\"val_tpl\":\"{{ value_json.outlet1 }}\","
             "\"dev\":%s"
           "}",
           FERDUINO_DEVICE_ID,
           cmdTopic,
           stateTopic,
           availTopic,
           device);

  publishRetained(topic, payload);
}

void publishDiscoveryMinimal() {
  publishSensorTwater();
  publishSwitchOutlet1();
}

void publishDiscoveryAll() {
  // B1: solo mínimo. En B2 se ampliará con el resto de entidades.
  publishDiscoveryMinimal();
}

} // namespace app::ha
