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
  Ip4 ip;
  Ip4 gateway;
  Ip4 subnet;
  Ip4 dns;
};

struct MqttConfig {
  char host[64];
  uint16_t port;
  char deviceId[48];
};

struct AppConfig {
  uint32_t magic;     // 'FDNX'
  uint16_t version;   // incrementa si cambia layout
  uint16_t size;      // sizeof(AppConfig)
  uint32_t crc32;     // CRC sobre el bloque (crc32=0 mientras se calcula)

  BackendMode backendMode;
  NetworkConfig net;
  MqttConfig mqtt;
};

// API
const AppConfig& get();              // acceso read-only al config en RAM
bool loadOrDefault();                // carga EEPROM o defaults (build_flags)
bool save();                         // escribe EEPROM (con CRC)
void factoryReset();                 // borra y vuelve a defaults en RAM

// Utilidades
void applyDefaults(AppConfig& cfg);  // rellena con build_flags
uint32_t computeCrc32(const void* data, size_t len);

} // namespace app::cfg
