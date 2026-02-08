#include "app/config/app_config.h"

#include <stdio.h>
#include <string.h>

#include "app/config/app_config_keys.h"
#include "app/nvm/eeprom_registry.h"

#ifndef FERDUINO_DEVICE_ID
  #define FERDUINO_DEVICE_ID "ferduino-next"
#endif

#ifndef MQTT_BROKER_HOST
  #define MQTT_BROKER_HOST "localhost"
#endif

#ifndef MQTT_BROKER_PORT
  #define MQTT_BROKER_PORT 1883
#endif

#ifndef FERDUINO_COMMS_MODE
  #define FERDUINO_COMMS_MODE FERDUINO_COMMS_LEGACY
#endif

namespace app::cfg {

static AppConfig g_cfg{};

// -----------------------------
// CRC32 (reutilizable para otros módulos)
// -----------------------------

static uint32_t crc32_update(uint32_t crc, uint8_t data) {
  crc ^= data;
  for (uint8_t i = 0; i < 8; i++) {
    uint32_t mask = -(crc & 1u);
    crc = (crc >> 1) ^ (0xEDB88320UL & mask);
  }
  return crc;
}

uint32_t computeCrc32(const void* data, size_t len) {
  const uint8_t* p = (const uint8_t*)data;
  uint32_t crc = 0xFFFFFFFFUL;
  for (size_t i = 0; i < len; i++) crc = crc32_update(crc, p[i]);
  return ~crc;
}

// -----------------------------
// Helpers packing IP
// -----------------------------

static uint32_t packIp(const Ip4& ip) {
  return (uint32_t)ip.a | ((uint32_t)ip.b << 8) | ((uint32_t)ip.c << 16) | ((uint32_t)ip.d << 24);
}

static Ip4 unpackIp(uint32_t u) {
  Ip4 ip{};
  ip.a = (uint8_t)(u & 0xFF);
  ip.b = (uint8_t)((u >> 8) & 0xFF);
  ip.c = (uint8_t)((u >> 16) & 0xFF);
  ip.d = (uint8_t)((u >> 24) & 0xFF);
  return ip;
}

static bool parseMac(const char* s, uint8_t out[6]) {
  // Formato esperado: "02:FD:00:00:00:01" (hex)
  if (!s) return false;
  unsigned int b[6] = {0};
  const int n = sscanf(s, "%2x:%2x:%2x:%2x:%2x:%2x", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]);
  if (n != 6) return false;
  for (int i = 0; i < 6; ++i) out[i] = (uint8_t)b[i];
  return true;
}

static void formatMac(const uint8_t mac[6], char* out, size_t outLen) {
  if (!out || outLen == 0) return;
  snprintf(out, outLen, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// -----------------------------
// Defaults desde build_flags
// -----------------------------

void applyDefaults(AppConfig& c) {
  memset(&c, 0, sizeof(c));

#if (FERDUINO_COMMS_MODE == FERDUINO_COMMS_HA)
  c.backendMode = BACKEND_HA;
#else
  c.backendMode = BACKEND_LEGACY;
#endif

  // MAC por defecto (local-admin)
  c.net.mac[0] = 0x02;
  c.net.mac[1] = 0xFD;
  c.net.mac[2] = 0x00;
  c.net.mac[3] = 0x00;
  c.net.mac[4] = 0x00;
  c.net.mac[5] = 0x01;

  c.net.useDhcp = true;
  c.net.ip      = {0, 0, 0, 0};
  c.net.gateway = {0, 0, 0, 0};
  c.net.subnet  = {255, 255, 255, 0};
  c.net.dns     = {0, 0, 0, 0};

  strncpy(c.mqtt.host, MQTT_BROKER_HOST, sizeof(c.mqtt.host) - 1);
  c.mqtt.host[sizeof(c.mqtt.host) - 1] = '\0';
  c.mqtt.port = (uint16_t)MQTT_BROKER_PORT;

  strncpy(c.mqtt.deviceId, FERDUINO_DEVICE_ID, sizeof(c.mqtt.deviceId) - 1);
  c.mqtt.deviceId[sizeof(c.mqtt.deviceId) - 1] = '\0';
}

const AppConfig& get() { return g_cfg; }

void set(const AppConfig& cfg) { g_cfg = cfg; }

// -----------------------------
// Load / Save vía registry TLV (NO toca EEPROM legacy 0..1023)
// -----------------------------

bool loadOrDefault() {
  applyDefaults(g_cfg);

  auto& reg = app::nvm::registry();
  (void)reg.begin();
  if (!reg.isValid()) {
    // Registry no inicializado -> lo dejamos en defaults.
    return false;
  }

  uint32_t u32 = 0;
  bool b = false;
  char tmp[64];

  if (reg.getU32(KEY_BACKEND_MODE, u32)) {
    g_cfg.backendMode = (u32 == 1) ? BACKEND_HA : BACKEND_LEGACY;
  }

  if (reg.getStr(KEY_MQTT_HOST, tmp, sizeof(tmp))) {
    strncpy(g_cfg.mqtt.host, tmp, sizeof(g_cfg.mqtt.host) - 1);
    g_cfg.mqtt.host[sizeof(g_cfg.mqtt.host) - 1] = '\0';
  }

  if (reg.getU32(KEY_MQTT_PORT, u32)) {
    g_cfg.mqtt.port = (uint16_t)u32;
  }

  if (reg.getStr(KEY_DEVICE_ID, tmp, sizeof(tmp))) {
    strncpy(g_cfg.mqtt.deviceId, tmp, sizeof(g_cfg.mqtt.deviceId) - 1);
    g_cfg.mqtt.deviceId[sizeof(g_cfg.mqtt.deviceId) - 1] = '\0';
  }

  if (reg.getBool(KEY_NET_DHCP, b)) {
    g_cfg.net.useDhcp = b;
  }

  if (reg.getU32(KEY_NET_IP, u32)) g_cfg.net.ip = unpackIp(u32);
  if (reg.getU32(KEY_NET_GW, u32)) g_cfg.net.gateway = unpackIp(u32);
  if (reg.getU32(KEY_NET_SUBNET, u32)) g_cfg.net.subnet = unpackIp(u32);
  if (reg.getU32(KEY_NET_DNS, u32)) g_cfg.net.dns = unpackIp(u32);

  if (reg.getStr(KEY_NET_MAC, tmp, sizeof(tmp))) {
    (void)parseMac(tmp, g_cfg.net.mac);
  }

  return true;
}

bool save() {
  auto& reg = app::nvm::registry();
  (void)reg.begin();
  if (!reg.isValid()) {
    if (!reg.format()) return false;
  }

  const uint32_t backend = (g_cfg.backendMode == BACKEND_HA) ? 1u : 0u;
  const uint32_t ip  = packIp(g_cfg.net.ip);
  const uint32_t gw  = packIp(g_cfg.net.gateway);
  const uint32_t sn  = packIp(g_cfg.net.subnet);
  const uint32_t dns = packIp(g_cfg.net.dns);

  char macStr[24];
  formatMac(g_cfg.net.mac, macStr, sizeof(macStr));

  if (!reg.beginEdit()) return false;

  (void)reg.setU32(KEY_BACKEND_MODE, backend);
  (void)reg.setStr(KEY_MQTT_HOST, g_cfg.mqtt.host);
  (void)reg.setU32(KEY_MQTT_PORT, (uint32_t)g_cfg.mqtt.port);
  (void)reg.setStr(KEY_DEVICE_ID, g_cfg.mqtt.deviceId);

  (void)reg.setBool(KEY_NET_DHCP, g_cfg.net.useDhcp);
  (void)reg.setU32(KEY_NET_IP, ip);
  (void)reg.setU32(KEY_NET_GW, gw);
  (void)reg.setU32(KEY_NET_SUBNET, sn);
  (void)reg.setU32(KEY_NET_DNS, dns);
  (void)reg.setStr(KEY_NET_MAC, macStr);

  return reg.endEdit();
}

void factoryReset() {
  auto& reg = app::nvm::registry();
  (void)reg.begin();
  (void)reg.format(); // deja registry vacío (y válido)
  applyDefaults(g_cfg);
  (void)save();
}

} // namespace app::cfg
