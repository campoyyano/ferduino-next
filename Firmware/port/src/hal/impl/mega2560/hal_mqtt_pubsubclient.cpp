#include "hal/hal_mqtt.h"
#include "hal/hal_network.h"

#if defined(ARDUINO_AVR_MEGA2560) || defined(ARDUINO_ARCH_AVR)

#include <Arduino.h>
#include <PubSubClient.h>

namespace hal {

/*
 * Wrapper: adapta ITcpClient a Arduino Client (requerido por PubSubClient).
 * Nota: este wrapper no hace buffering; PubSubClient ya gestiona su propio buffer.
 */
class TcpClientAsArduinoClient final : public Client {
public:
  explicit TcpClientAsArduinoClient(ITcpClient& c) : _c(c) {}

  int connect(IPAddress ip, uint16_t port) override {
    char buf[24];
    snprintf(buf, sizeof(buf), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
    return _c.connect(buf, port) ? 1 : 0;
  }

  int connect(const char* host, uint16_t port) override {
    return _c.connect(host, port) ? 1 : 0;
  }

  size_t write(uint8_t b) override {
    return _c.write(&b, 1);
  }

  size_t write(const uint8_t* buf, size_t size) override {
    return _c.write(buf, size);
  }

  int available() override {
    return _c.available();
  }

  int read() override {
    return _c.read();
  }

  int read(uint8_t* buf, size_t size) override {
    return _c.read(buf, size);
  }

  int peek() override {
    // No implementamos peek (si una versión de PubSubClient lo exige, se ajusta).
    return -1;
  }

  void flush() override {
    _c.flush();
  }

  void stop() override {
    _c.stop();
  }

  uint8_t connected() override {
    return _c.connected() ? 1 : 0;
  }

  operator bool() override { return true; }

private:
  ITcpClient& _c;
};

static MqttMessageCb g_userCb = nullptr;

static void pubsubCallback(char* topic, uint8_t* payload, unsigned int length) {
  if (!g_userCb) return;
  g_userCb(topic, payload, (size_t)length);
}

class MqttHalPubSubClient final : public IMqttHal {
public:
  MqttError begin(const MqttConfig& cfg) override {
    if (!cfg.host || !cfg.clientId) return MqttError::InvalidConfig;

    _cfg = cfg;
    _initialized = true;

    _ps.setServer(_cfg.host, _cfg.port);
    _ps.setCallback(pubsubCallback);
    _ps.setKeepAlive(_cfg.keepAliveSec);

    return MqttError::Ok;
  }

  MqttError connect() override {
    if (!_initialized) return MqttError::NotInitialized;
    if (!network().isInitialized()) return MqttError::NetworkNotReady;

    // --- B5.6: LWT autogenerado si no viene configurado ---
    const char* willTopic = _cfg.willTopic;
    const char* willPayload = _cfg.willPayload;
    uint8_t willQos = _cfg.willQos;
    bool willRetained = _cfg.willRetained;

    if (!willTopic || willTopic[0] == '\0') {
      // Default: ferduino/<clientId>/status  retained  {"online":0}
      snprintf(_autoWillTopic, sizeof(_autoWillTopic),
               "ferduino/%s/status", _cfg.clientId);
      strncpy(_autoWillPayload, "{\"online\":0}", sizeof(_autoWillPayload));
      _autoWillPayload[sizeof(_autoWillPayload) - 1] = '\0';

      willTopic = _autoWillTopic;
      willPayload = _autoWillPayload;
      willQos = 0;
      willRetained = true;
    }
    if (!willPayload) {
      // payload mínimo si el topic existe pero no hay payload
      strncpy(_autoWillPayload, "{\"online\":0}", sizeof(_autoWillPayload));
      _autoWillPayload[sizeof(_autoWillPayload) - 1] = '\0';
      willPayload = _autoWillPayload;
    }

    bool ok = false;

    const bool useAuth = (_cfg.user && _cfg.user[0] != '\0' &&
                          _cfg.pass && _cfg.pass[0] != '\0');

    const bool useWill = (willTopic && willTopic[0] != '\0');

    if (useWill) {
      if (useAuth) {
        ok = _ps.connect(_cfg.clientId, _cfg.user, _cfg.pass,
                         willTopic, willQos, willRetained, willPayload);
      } else {
        ok = _ps.connect(_cfg.clientId,
                         willTopic, willQos, willRetained, willPayload);
      }
    } else {
      if (useAuth) {
        ok = _ps.connect(_cfg.clientId, _cfg.user, _cfg.pass);
      } else {
        ok = _ps.connect(_cfg.clientId);
      }
    }

    return ok ? MqttError::Ok : MqttError::ConnectFailed;
  }

  void disconnect() override {
    _ps.disconnect();
    network().tcp().stop();
  }

  bool connected() const override {
    return const_cast<PubSubClient&>(_ps).connected();
  }

  void loop() override {
    _ps.loop();
  }

  MqttError publish(const char* topic,
                    const uint8_t* payload,
                    size_t len,
                    bool retained = false) override {
    if (!topic || (!payload && len > 0)) return MqttError::InvalidConfig;
    if (!_ps.connected()) return MqttError::ConnectFailed;

    const bool ok = _ps.publish(topic, payload, (unsigned int)len, retained);
    return ok ? MqttError::Ok : MqttError::PublishFailed;
  }

  MqttError subscribe(const char* topic, uint8_t qos = 0) override {
    (void)qos;
    if (!topic) return MqttError::InvalidConfig;
    if (!_ps.connected()) return MqttError::ConnectFailed;

    const bool ok = _ps.subscribe(topic);
    return ok ? MqttError::Ok : MqttError::SubscribeFailed;
  }

  void onMessage(MqttMessageCb cb) override {
    g_userCb = cb;
  }

private:
  bool _initialized = false;
  MqttConfig _cfg{};

  // LWT por defecto (B5.6)
  char _autoWillTopic[96] = {0};
  char _autoWillPayload[32] = {0};

  // El wrapper se construye UNA vez con el TCP de la HAL de red
  TcpClientAsArduinoClient _client{ network().tcp() };
  PubSubClient _ps{ _client };
};

static MqttHalPubSubClient g_mqtt;

IMqttHal& mqtt() {
  return g_mqtt;
}

} // namespace hal

#endif // AVR
