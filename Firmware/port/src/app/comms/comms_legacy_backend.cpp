#include "app/comms_backend.h"
#include "app/comms_mode.h"

#include "hal/hal_mqtt.h"
#include "hal/hal_network.h"

#include <Arduino.h>
#include <string.h>
#include <stdlib.h>

/* ==== Build flags obligatorios ==== */

#ifndef FERDUINO_LEGACY_USERNAME
  #error "Define FERDUINO_LEGACY_USERNAME in platformio.ini build_flags"
#endif

#ifndef FERDUINO_LEGACY_APIKEY
  #error "Define FERDUINO_LEGACY_APIKEY in platformio.ini build_flags"
#endif

#ifndef FERDUINO_DEVICE_ID
  #error "Define FERDUINO_DEVICE_ID in platformio.ini build_flags"
#endif

#ifndef MQTT_BROKER_HOST
  #error "Define MQTT_BROKER_HOST in platformio.ini build_flags"
#endif

#ifndef MQTT_BROKER_PORT
  #error "Define MQTT_BROKER_PORT in platformio.ini build_flags"
#endif

namespace app {

/* ==== helpers ==== */

static void makeTopic(char* out, size_t outLen, const char* suffix) {
  // Legacy format: Username/APIKEY/<suffix>
  snprintf(out, outLen,
           "%s/%s/%s",
           FERDUINO_LEGACY_USERNAME,
           FERDUINO_LEGACY_APIKEY,
           suffix);
}

/* ==== Backend Legacy ==== */

class CommsLegacyBackend final : public ICommsBackend {
public:
  void begin() override {
    hal::MqttConfig cfg;
    cfg.host = MQTT_BROKER_HOST;
    cfg.port = (uint16_t)MQTT_BROKER_PORT;
    cfg.clientId = FERDUINO_DEVICE_ID;
    cfg.keepAliveSec = 30;

    (void)hal::mqtt().begin(cfg);
    hal::mqtt().onMessage(&CommsLegacyBackend::onMqttMessageStatic);

    makeTopic(_subTopic, sizeof(_subTopic), "topic/command");
    makeTopic(_pubTopic, sizeof(_pubTopic), "topic/response");

    _lastReconnectMs = 0;
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
        (void)hal::mqtt().subscribe(_subTopic);

        const char online[] = "{\"online\":1}";
        (void)hal::mqtt().publish(
          _pubTopic,
          (const uint8_t*)online,
          sizeof(online) - 1,
          true
        );
      }
    }
  }

  bool connected() const override {
    return hal::mqtt().connected();
  }

  bool publishStatus(const char* key,
                     const char* value,
                     bool retained=false) override {
    if (!key || !value) return false;

    char msg[96];
    snprintf(msg, sizeof(msg),
             "{\"%s\":\"%s\"}", key, value);

    return hal::mqtt().publish(
      _pubTopic,
      (const uint8_t*)msg,
      strlen(msg),
      retained
    ) == hal::MqttError::Ok;
  }

  void onMqttMessage(const char*,
                     const uint8_t* payload,
                     size_t len) override {
    if (!payload || len == 0)
      return;

    const size_t max = sizeof(_rxBuf) - 1;
    const size_t n = (len > max) ? max : len;
    memcpy(_rxBuf, payload, n);
    _rxBuf[n] = '\0';

    // Legacy terminator 'K'
    char* kpos = strchr(_rxBuf, 'K');
    if (!kpos)
      return;

    if (kpos > _rxBuf)
      *(kpos - 1) = '\0';

    int argc = 0;
    char* save = nullptr;
    char* p = _rxBuf;

    while (argc < (int)MAX_TOKENS) {
      char* tok = strtok_r(p, ",", &save);
      if (!tok) break;
      _argv[argc++] = tok;
      p = nullptr;
    }

    if (argc <= 0)
      return;

    const int id = atoi(_argv[0]);

    switch (id) {
      case 0:
        publishHomeStub();
        break;

      default:
        publishUnknown(id);
        break;
    }
  }

  static void onMqttMessageStatic(const char* topic,
                                 const uint8_t* payload,
                                 size_t len) {
    instance().onMqttMessage(topic, payload, len);
  }

  static CommsLegacyBackend& instance() {
    static CommsLegacyBackend inst;
    return inst;
  }

private:
  void publishHomeStub() {
    char msg[160];
    snprintf(msg, sizeof(msg),
      "{\"id\":0,\"uptime\":%lu,\"ip\":\"%u.%u.%u.%u\"}",
      (unsigned long)(millis()/1000),
      hal::network().localIp().a,
      hal::network().localIp().b,
      hal::network().localIp().c,
      hal::network().localIp().d
    );

    (void)hal::mqtt().publish(
      _pubTopic,
      (const uint8_t*)msg,
      strlen(msg),
      false
    );
  }

  void publishUnknown(int id) {
    char msg[64];
    snprintf(msg, sizeof(msg),
             "{\"error\":\"unknown_id\",\"id\":%d}", id);

    (void)hal::mqtt().publish(
      _pubTopic,
      (const uint8_t*)msg,
      strlen(msg),
      false
    );
  }

private:
  static constexpr size_t RX_BUF_SIZE = 96;
  static constexpr size_t MAX_TOKENS  = 12;

  char _subTopic[64] = {0};
  char _pubTopic[64] = {0};

  uint32_t _lastReconnectMs = 0;

  char _rxBuf[RX_BUF_SIZE] = {0};
  char* _argv[MAX_TOKENS]  = {0};
};

/* ==== Factory hook ==== */

ICommsBackend& comms_legacy_singleton() {
  return CommsLegacyBackend::instance();
}

} // namespace app
