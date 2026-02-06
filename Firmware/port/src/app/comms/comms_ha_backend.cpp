#include "app/comms/comms_ha_backend.h"

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

    // Suscripciones de comandos (B1: solo outlet/1)
    snprintf(_cmdOutlet1Topic, sizeof(_cmdOutlet1Topic), "%s/%s/cmd/outlet/1", BASE_TOPIC, FERDUINO_DEVICE_ID);

    _lastReconnectMs = 0;
    _discoveryPublished = false;
  }

  void loop() override {
    hal::network().loop();
    hal::mqtt().loop();

    if (!hal::network().isInitialized())
      return;

    if (!hal::mqtt().connected()) {
      const uint32_t now = millis();
      if (now - _lastReconnectMs < 2000)
        return;
      _lastReconnectMs = now;

      if (hal::mqtt().connect() == hal::MqttError::Ok) {
        // Availability online retained
        publishAvailability(true);

        // Subscribe comandos
        (void)hal::mqtt().subscribe(_cmdOutlet1Topic);

        // Discovery (una vez por arranque/conexión)
        if (!_discoveryPublished) {
          app::ha::publishDiscoveryMinimal();
          _discoveryPublished = true;
        }

        // Publica primer estado placeholder
        publishStatePlaceholder();
      }
      return;
    }

    // B1: refresco ligero de estado (placeholder) cada 5s para ver el flujo en logs
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
    // B1: helper mínimo para publicar un estado JSON de 1 clave.
    // En B2 haremos un state JSON coherente de todo el sistema.
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
    // B1: solo “outlet1”
    if (!topic || !payload || len == 0) return;

    // Copia payload a buffer
    const size_t n = (len >= sizeof(_rx)) ? (sizeof(_rx) - 1) : len;
    memcpy(_rx, payload, n);
    _rx[n] = '\0';

    if (strcmp(topic, _cmdOutlet1Topic) == 0) {
      // HA enviará "1" / "0". Sin placa, no actuamos: solo reflejamos en estado.
      // Guardamos el “estado simulado”.
      _outlet1 = (_rx[0] == '1') ? 1 : 0;

      // Publica estado actualizado (placeholder)
      publishStatePlaceholder();
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
    // Placeholder compatible con discovery B1:
    // - twater para sensor
    // - outlet1 para switch
    const float twater = 0.0f;

    char msg[220];
    // twater float -> imprime como double por variádicos
    snprintf(msg, sizeof(msg),
             "{"
               "\"twater\":%.2f,"
               "\"outlet1\":%d,"
               "\"running\":%lu"
             "}",
             (double)twater,
             _outlet1,
             (unsigned long)(millis() / 1000UL));

    (void)hal::mqtt().publish(_stateTopic, (const uint8_t*)msg, strlen(msg), false);
  }

private:
  uint32_t _lastReconnectMs = 0;
  uint32_t _lastStateMs = 0;

  bool _discoveryPublished = false;

  char _stateTopic[96] = {0};
  char _availTopic[96] = {0};
  char _cmdOutlet1Topic[96] = {0};

  char _rx[32] = {0};

  // estado simulado (sin placa)
  int _outlet1 = 0;
};

ICommsBackend& comms_ha_singleton() {
  return CommsHABackend::instance();
}

} // namespace app
