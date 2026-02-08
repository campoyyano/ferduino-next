#pragma once

#include <stdint.h>
#include <stddef.h>

namespace app::cfg {

enum BackendMode : uint8_t {
  BACKEND_LEGACY = 0,
  BACKEND_HA     = 1,
};

struct Ip4 {
  uint8_t a, b, c, d;
};

struct NetworkConfig {
  uint8_t mac[6];
  bool useDhcp;

  // Si useDhcp == false:
  Ip4 ip;
  Ip4 gateway;
  Ip4 subnet;
  Ip4 dns;
};

struct MqttConfig {
  const char* host;
  uint16_t port;

  const char* deviceId;   // topic base ferduino/<deviceId>/...
  const char* clientId;   // MQTT client id (puede coincidir o no con deviceId)

  const char* username;   // legacy (si aplica)
  const char* apikey;     // legacy (si aplica)
};

struct AppConfig {
  uint16_t version;   // schema version
  uint16_t size;      // sizeof(AppConfig)
  uint32_t crc32;     // CRC sobre el bloque (crc32=0 mientras se calcula)

  BackendMode backendMode;
  NetworkConfig net;
  MqttConfig mqtt;
};

// API
const AppConfig& get();              // acceso read-only al config en RAM
bool loadOrDefault();                // carga desde NVM registry (TLV) o aplica defaults (build_flags)
bool save();                         // guarda en NVM registry (TLV) con CRC
void factoryReset();                 // borra keys en NVM registry y vuelve a defaults en RAM

// NUEVO B3.4: aplicar config en RAM (sin guardar aún)
void set(const AppConfig& cfg);

void applyDefaults(AppConfig& cfg);  // rellena con build_flags
uint32_t computeCrc32(const void* data, size_t len);

} // namespace app::cfg
