#include "app/comms/comms_ha_backend.h"
#include "app/comms/ha/ha_outlet_control.h"
#include "app/comms/ha/ha_discovery.h"

#include "hal/hal_mqtt.h"
#include "hal/hal_network.h"

#include <Arduino.h>
#include <string.h>
#include <stdio.h>

#ifndef MQTT_BROKER_HOST
  #error "MQTT_BROKER_HOST must be defined in platformio.ini build_flags"
#endif

#ifndef MQTT_BROKER_PORT
  #error "MQTT_BROKER_PORT must be defined in platformio.ini build_flags"
#endif

#ifndef FERDUINO_DEVICE_ID
  #error "FERDUINO_DEVICE_ID must be defined in platformio.ini build_flags"
#endif

namespace app {

static constexpr const char* BASE_TOPIC = "ferduino";

class CommsHABackend final : public ICommsBackend {
public:
  void begin() override {
    hal::MqttConfig cfg;
    cfg.host = MQTT_BROKER_HOST;
    cfg.port = (uint16_t)MQTT_BROKER_PORT;
    cfg.clientId = FERDUINO_DEVICE_ID;
    cfg.keepAliveSec = 30;

    (void)hal::mqtt().begin(cfg);
    hal::mqtt().onMessage(&CommsHABackend::onMqttMessageStatic);

    snprintf(_stateTopic, sizeof(_stateTopic), "%s/%s/state", BASE_TOPIC, FERDUINO_DEVICE_ID);
    snprintf(_availTopic, sizeof(_availTopic), "%s/%s/availability", BASE_TOPIC, FERDUINO_DEVICE_ID);

    // Suscripciones de comandos: outlets 1..9 (B2.3)
    for (int i = 1; i <= 9; i++) {
      snprintf(_cmdOutletTopic[i], sizeof(_cmdOutletTopic[i]),
               "%s/%s/cmd/outlet/%d", BASE_TOPIC, FERDUINO_DEVICE_ID, i);
    }

    _lastReconnectMs = 0;
    _lastStateMs = 0;
    _discoveryPublished = false;
  }

  void loop() override {
    hal::network().loop();
    hal::mqtt().loop();

    if (!hal::network().isInitialized()) {
      return;
    }

    if (!hal::mqtt().connected()) {
      const uint32_t now = millis();
      if (now - _lastReconnectMs < 2000) {
        return;
      }
      _lastReconnectMs = now;

      if (hal::mqtt().connect() == hal::MqttError::Ok) {
        // Availability online retained
        publishAvailability(true);

        // Subscribe comandos outlets 1..9
        for (int i = 1; i <= 9; i++) {
          (void)hal::mqtt().subscribe(_cmdOutletTopic[i]);
        }

        // Discovery (una vez por arranque/conexión)
        if (!_discoveryPublished) {
          app::ha::publishDiscoveryMinimal();
          _discoveryPublished = true;
        }

        // Publica estado inicial (placeholder coherente con HA discovery)
        publishStatePlaceholder();
      }
      return;
    }

    // Refresco ligero de estado cada 5s (placeholder) para observabilidad
    const uint32_t now = millis();
    if (now - _lastStateMs > 5000) {
      _lastStateMs = now;
      publishStatePlaceholder();
    }
  }

  bool connected() const override {
    return hal::mqtt().connected();
  }

  bool publishStatus(const char* key, const char* value, bool retained=false) override {
    // Helper mínimo: publica JSON de 1 clave. Se mantiene por compatibilidad con la interfaz.
    if (!key || !value) return false;

    char msg[180];
    snprintf(msg, sizeof(msg), "{\"%s\":\"%s\"}", key, value);

    return hal::mqtt().publish(
      _stateTopic,
      (const uint8_t*)msg,
      strlen(msg),
      retained
    ) == hal::MqttError::Ok;
  }

  void onMqttMessage(const char* topic, const uint8_t* payload, size_t len) override {
    if (!topic || !payload || len == 0) return;

    // Copia payload a buffer
    const size_t n = (len >= sizeof(_rx)) ? (sizeof(_rx) - 1) : len;
    memcpy(_rx, payload, n);
    _rx[n] = '\0';

    // Outlets 1..9
    for (int i = 1; i <= 9; i++) {
      if (strcmp(topic, _cmdOutletTopic[i]) == 0) {
        const uint8_t v = (_rx[0] == '1') ? 1 : 0;
        app::ha::applyOutletCommand((uint8_t)i, v);

        // Publica estado actualizado coherente con HA discovery
        publishStatePlaceholder();
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
    // Publica un state mínimo coherente con el discovery (B2.x):
    // - water_temperature, heatsink_temperature, ambient_temperature (placeholder)
    // - water_ph, reactor_ph, orp, salinity (placeholder)
    // - led_*_power (placeholder)
    // - outlet_1..outlet_9 (estado HA simulado)
    // - uptime (segundos)
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
             (int)app::ha::getOutletState(1),
             (int)app::ha::getOutletState(2),
             (int)app::ha::getOutletState(3),
             (int)app::ha::getOutletState(4),
             (int)app::ha::getOutletState(5),
             (int)app::ha::getOutletState(6),
             (int)app::ha::getOutletState(7),
             (int)app::ha::getOutletState(8),
             (int)app::ha::getOutletState(9),
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

  char _rx[32] = {0};
};

ICommsBackend& comms_ha_singleton() {
  return CommsHABackend::instance();
}

} // namespace app
