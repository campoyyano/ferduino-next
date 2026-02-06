#pragma once

#include <stdint.h>
#include <stddef.h>

namespace app {

class ICommsBackend {
public:
  virtual ~ICommsBackend() = default;

  virtual void begin() = 0;
  virtual void loop() = 0;

  virtual bool connected() const = 0;

  // Publicar estado/telemetría de forma abstracta (lo concretaremos luego)
  virtual bool publishStatus(const char* key, const char* value, bool retained=false) = 0;

  // Hook opcional: para reenviar eventos (lo concretaremos en legacy/HA)
  virtual void onMqttMessage(const char* topic, const uint8_t* payload, size_t len) = 0;
};

ICommsBackend& comms();

} // namespace app
