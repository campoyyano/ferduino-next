#pragma once

#include <stdint.h>
#include <stddef.h>

// HAL de MQTT (cliente).
// Nota: este header NO debe incluir headers de plataforma ni librerías concretas (PubSubClient, etc.).
// La implementación vive en src/hal/impl/<platform>/.

namespace hal {

struct MqttConfig {
  const char* host = nullptr;
  uint16_t port = 1883;

  const char* clientId = nullptr;
  const char* user = nullptr;
  const char* pass = nullptr;

  uint16_t keepAliveSec = 30;
  bool cleanSession = true;

  // LWT (Last Will & Testament)
  // Si willTopic == nullptr, la implementación puede autogenerar uno por defecto.
  const char* willTopic = nullptr;      // e.g. "ferduino/<deviceId>/status"
  const char* willPayload = nullptr;    // e.g. "{\"online\":0}"
  uint8_t willQos = 0;
  bool willRetained = true;
};

enum class MqttError : uint8_t {
  Ok = 0,
  NotInitialized,
  InvalidConfig,
  NetworkNotReady,
  ConnectFailed,
  PublishFailed,
  SubscribeFailed,
  InternalError,
};

using MqttMessageCb = void(*)(const char* topic, const uint8_t* payload, size_t len);

class IMqttHal {
public:
  virtual ~IMqttHal() = default;

  // Configura el cliente (host/puerto/credenciales/etc).
  virtual MqttError begin(const MqttConfig& cfg) = 0;

  // Conecta al broker. Debe ser rápida (sin bloqueos largos).
  virtual MqttError connect() = 0;

  virtual void disconnect() = 0;
  virtual bool connected() const = 0;

  // Mantiene keepalive y procesa mensajes entrantes.
  virtual void loop() = 0;

  virtual MqttError publish(const char* topic,
                            const uint8_t* payload,
                            size_t len,
                            bool retained = false) = 0;

  virtual MqttError subscribe(const char* topic, uint8_t qos = 0) = 0;

  virtual void onMessage(MqttMessageCb cb) = 0;
};

// Service locator (instancia MQTT para la plataforma actual)
IMqttHal& mqtt();

} // namespace hal
