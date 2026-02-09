#include "hal/hal_mqtt.h"
#include "hal/hal_network.h"

#if defined(ARDUINO_AVR_MEGA2560) || defined(ARDUINO_ARCH_AVR)

#include <Arduino.h>
#include <PubSubClient.h>

namespace hal {

/*
 * Wrapper: adapta ITcpClient a Arduino Client (requerido por PubSubClient).
 * Implementa peek() con cache de 1 byte para compatibilidad.
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
    const int a = _c.available();
    return _peekValid ? (a + 1) : a;
  }

  int read() override {
    if (_peekValid) {
      _peekValid = false;
      return (int)_peekByte;
    }
    return _c.read();
  }

  int read(uint8_t* buf, size_t size) override {
    if (!buf || size == 0) return 0;

    if (_peekValid) {
      buf[0] = _peekByte;
      _peekValid = false;
      if (size == 1) return 1;

      const int r = _c.read(buf + 1, size - 1);
      if (r < 0) return 1;
      return r + 1;
    }

    return _c.read(buf, size);
  }

  int peek() override {
    if (_peekValid) return (int)_peekByte;

    if (_c.available() <= 0) return -1;

    const int v = _c.read();
    if (v < 0) return -1;

    _peekByte = (uint8_t)v;
    _peekValid = true;
    return v;
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
  bool _peekValid = false;
  uint8_t _peekByte = 0;
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

    // --- LWT autogenerado si no viene configurado ---
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

  void loop() override {
    if (!_initialized) return;

    if (!_tcpWrapped) {
      _tcpWrapped = true;
      _tcpClient = &network().tcp();
      _clientWrapper = new TcpClientAsArduinoClient(*_tcpClient);
      _ps.setClient(*_clientWrapper);
    }

    (void)_ps.loop();
  }

  bool connected() const override {
    // PubSubClient::connected() no es const -> cast seguro (no muta estado observable)
    return const_cast<PubSubClient&>(_ps).connected();
  }

  void disconnect() override {
    if (!_initialized) return;
    _ps.disconnect();
  }

  MqttError publish(const char* topic, const uint8_t* payload, size_t len, bool retained) override {
    if (!_initialized) return MqttError::NotInitialized;
    if (!topic || topic[0] == '\0') return MqttError::InvalidConfig;
    if (!payload || len == 0) return MqttError::InvalidConfig;

    const bool ok = _ps.publish(topic, payload, (unsigned int)len, retained);
    return ok ? MqttError::Ok : MqttError::PublishFailed;
  }

  MqttError subscribe(const char* topic, uint8_t qos = 0) override {
    if (!_initialized) return MqttError::NotInitialized;
    if (!topic || topic[0] == '\0') return MqttError::InvalidConfig;

    const bool ok = _ps.subscribe(topic, qos);
    return ok ? MqttError::Ok : MqttError::SubscribeFailed;
  }

  void onMessage(MqttMessageCb cb) override {
    g_userCb = cb;
  }

private:
  MqttConfig _cfg{};
  bool _initialized = false;

  PubSubClient _ps;

  bool _tcpWrapped = false;
  ITcpClient* _tcpClient = nullptr;
  TcpClientAsArduinoClient* _clientWrapper = nullptr;

  char _autoWillTopic[96] = {0};
  char _autoWillPayload[32] = {0};
};

IMqttHal& mqtt() {
  static MqttHalPubSubClient s;
  return s;
}

} // namespace hal

#endif // MEGA2560 / AVR
