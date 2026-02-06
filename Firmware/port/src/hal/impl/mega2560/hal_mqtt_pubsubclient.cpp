#include "hal/hal_mqtt.h"
#include "hal/hal_network.h"

#if defined(ARDUINO_AVR_MEGA2560) || defined(ARDUINO_ARCH_AVR)

#include <Arduino.h>
#include <PubSubClient.h>

namespace hal {

/*
 * Wrapper: adapta ITcpClient a Arduino Client (requerido por PubSubClient)
 * Incluye implementación segura de peek()
 */
class TcpClientAsArduinoClient final : public Client {
public:
  explicit TcpClientAsArduinoClient(ITcpClient& tcp)
    : _tcp(tcp) {}

  int connect(const char* host, uint16_t port) override {
    return _tcp.connect(host, port) ? 1 : 0;
  }

  int connect(IPAddress ip, uint16_t port) override {
    (void)ip;
    (void)port;
    return 0; // no usado
  }

  size_t write(uint8_t b) override {
    return _tcp.write(&b, 1);
  }

  size_t write(const uint8_t* buf, size_t size) override {
    return _tcp.write(buf, size);
  }

  int available() override {
    return _tcp.available();
  }

  int peek() override {
    if (_hasPeek) return _peekByte;
    if (_tcp.available() <= 0) return -1;

    const int c = _tcp.read();
    if (c < 0) return -1;

    _peekByte = c;
    _hasPeek = true;
    return _peekByte;
  }

  int read() override {
    if (_hasPeek) {
      _hasPeek = false;
      return _peekByte;
    }
    return _tcp.read();
  }

  int read(uint8_t* buf, size_t size) override {
    if (!buf || size == 0) return 0;

    size_t n = 0;
    if (_hasPeek) {
      buf[0] = (uint8_t)_peekByte;
      _hasPeek = false;
      n = 1;
      if (size == 1) return 1;
    }

    const int r = _tcp.read(buf + n, size - n);
    return (r < 0) ? (int)n : (int)(n + (size_t)r);
  }

  void flush() override {
    _tcp.flush();
  }

  void stop() override {
    _tcp.stop();
  }

  uint8_t connected() override {
    return _tcp.connected() ? 1 : 0;
  }

  operator bool() override {
    return connected();
  }

private:
  ITcpClient& _tcp;
  bool _hasPeek = false;
  int  _peekByte = -1;
};

// Callback global (PubSubClient lo exige así)
static MqttMessageCb g_userCb = nullptr;

static void pubsubCallback(char* topic, uint8_t* payload, unsigned int length) {
  if (g_userCb) {
    g_userCb(topic, payload, (size_t)length);
  }
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

    bool ok = false;
    if (_cfg.user && _cfg.pass) {
      ok = _ps.connect(_cfg.clientId, _cfg.user, _cfg.pass);
    } else {
      ok = _ps.connect(_cfg.clientId);
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

  // El wrapper se construye UNA vez con el TCP de la HAL de red
  TcpClientAsArduinoClient _client{ network().tcp() };
  PubSubClient _ps{ _client };
};

static MqttHalPubSubClient g_mqtt;

IMqttHal& mqtt() {
  return g_mqtt;
}

} // namespace hal

#endif

