#include "app/comms/comms_ha_backend.h"

#include "app/config/app_config.h"
#include "app/config/app_config_mqtt_admin.h"

#include "app/comms/ha/ha_discovery.h"
#include "app/outlets/app_outlets.h"

#include "hal/hal_mqtt.h"
#include "hal/hal_network.h"

#include <Arduino.h>
#include <string.h>
#include <stdio.h>

namespace app {

static constexpr const char* BASE_TOPIC = "ferduino";

class CommsHABackend final : public ICommsBackend {
public:
  void begin() override {
    // No hay setup() global todavía -> garantizamos cfg en RAM
    (void)app::cfg::loadOrDefault();

    const auto& cfg = app::cfg::get();
    const char* deviceId = cfg.mqtt.deviceId;

    hal::MqttConfig mcfg;
    mcfg.host = cfg.mqtt.host;
    mcfg.port = cfg.mqtt.port;
    mcfg.clientId = deviceId;
    mcfg.keepAliveSec = 30;

    (void)hal::mqtt().begin(mcfg);
    hal::mqtt().onMessage(&CommsHABackend::onMqttMessageStatic);

    snprintf(_stateTopic, sizeof(_stateTopic), "%s/%s/state", BASE_TOPIC, deviceId);
    snprintf(_availTopic, sizeof(_availTopic), "%s/%s/availability", BASE_TOPIC, deviceId);

    for (int i = 1; i <= 9; i++) {
      snprintf(_cmdOutletTopic[i], sizeof(_cmdOutletTopic[i]),
               "%s/%s/cmd/outlet/%d", BASE_TOPIC, deviceId, i);
      (void)hal::mqtt().subscribe(_cmdOutletTopic[i]);
    }

    // Config admin topic
    snprintf(_cmdCfgTopic, sizeof(_cmdCfgTopic), "%s/%s/cmd/config", BASE_TOPIC, deviceId);
    (void)hal::mqtt().subscribe(_cmdCfgTopic);

    // Discovery (retained)
    if (!_discoveryPublished) {
      app::ha::publishDiscoveryMinimal();
      _discoveryPublished = true;
    }

    // Estado inicial
    publishStatePlaceholder();
    publishAvailability(true);

    _lastReconnectMs = millis();
  }

  void loop() override {
    const uint32_t now = millis();

    // reconexión simple si MQTT cae
    if (!hal::mqtt().connected()) {
      if (now - _lastReconnectMs > 3000) {
        _lastReconnectMs = now;
        (void)hal::mqtt().reconnect();
      }
      return;
    }

    // publish periódico de estado (placeholder)
    if (now - _lastStateMs > 5000) {
      _lastStateMs = now;
      publishStatePlaceholder();
    }
  }

  void onMqttMessage(const char* topic, const uint8_t* payload, size_t len) override {
    if (!topic || !payload || len == 0) return;

    // B3.4: admin config primero
    if (app::cfgadmin::handleConfigCommand(topic, payload, len)) {
      return;
    }

    // Copia payload
    const size_t n = (len >= sizeof(_rx)) ? (sizeof(_rx) - 1) : len;
    memcpy(_rx, payload, n);
    _rx[n] = '\0';

    // Outlets 1..9
    for (int i = 1; i <= 9; i++) {
      if (strcmp(topic, _cmdOutletTopic[i]) == 0) {
        const bool on = (_rx[0] == '1');
        app::outlets::set((uint8_t)(i - 1), on);
        publishStatePlaceholder(); // refleja el cambio inmediatamente
        return;
      }
    }
  }

  static void onMqttMessageStatic(const char* topic, const uint8_t* payload, size_t len) {
    instance().onMqttMessage(topic, payload, len);
  }

  static CommsHABackend& instance() {
    static CommsHABackend inst;
    return inst;
  }

private:
  void publishAvailability(bool online) {
    const char* v = online ? "online" : "offline";
    (void)hal::mqtt().publish(_availTopic, (const uint8_t*)v, strlen(v), true);
  }

  void publishStatePlaceholder() {
    // Estado mínimo coherente con discovery B3.x
    const float water_temperature    = 0.0f;
    const float heatsink_temperature = 0.0f;
    const float ambient_temperature  = 0.0f;

    const float water_ph   = 0.0f;
    const float reactor_ph = 0.0f;
    const float orp        = 0.0f;
    const float salinity   = 0.0f;

    const int led_white = 0;
    const int led_blue  = 0;
    const int led_rb    = 0;
    const int led_red   = 0;
    const int led_uv    = 0;

    char msg[640];
    snprintf(msg, sizeof(msg),
             "{"
               "\"water_temperature\":%.2f,"
               "\"heatsink_temperature\":%.2f,"
               "\"ambient_temperature\":%.2f,"
               "\"water_ph\":%.2f,"
               "\"reactor_ph\":%.2f,"
               "\"orp\":%.2f,"
               "\"salinity\":%.3f,"
               "\"led_white_power\":%d,"
               "\"led_blue_power\":%d,"
               "\"led_royal_blue_power\":%d,"
               "\"led_red_power\":%d,"
               "\"led_uv_power\":%d,"
               "\"outlet_1\":%d,"
               "\"outlet_2\":%d,"
               "\"outlet_3\":%d,"
               "\"outlet_4\":%d,"
               "\"outlet_5\":%d,"
               "\"outlet_6\":%d,"
               "\"outlet_7\":%d,"
               "\"outlet_8\":%d,"
               "\"outlet_9\":%d,"
               "\"uptime\":%lu"
             "}",
             (double)water_temperature,
             (double)heatsink_temperature,
             (double)ambient_temperature,
             (double)water_ph,
             (double)reactor_ph,
             (double)orp,
             (double)salinity,
             led_white,
             led_blue,
             led_rb,
             led_red,
             led_uv,
             (int)app::outlets::get(0),
             (int)app::outlets::get(1),
             (int)app::outlets::get(2),
             (int)app::outlets::get(3),
             (int)app::outlets::get(4),
             (int)app::outlets::get(5),
             (int)app::outlets::get(6),
             (int)app::outlets::get(7),
             (int)app::outlets::get(8),
             (unsigned long)(millis() / 1000UL));

    (void)hal::mqtt().publish(_stateTopic, (const uint8_t*)msg, strlen(msg), false);
  }

private:
  uint32_t _lastReconnectMs = 0;
  uint32_t _lastStateMs = 0;

  bool _discoveryPublished = false;

  char _stateTopic[96] = {0};
  char _availTopic[96] = {0};

  // cmd topics outlets 1..9 (índice 1..9)
  char _cmdOutletTopic[10][96] = {{0}};
  char _cmdCfgTopic[96] = {0};

  // rx buffer
  char _rx[96] = {0};
};

ICommsBackend& commsHABackend() {
  return CommsHABackend::instance();
}

} // namespace app
