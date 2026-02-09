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
// CRC32
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
  for (size_t i = 0; i < len; i++) {
    crc = crc32_update(crc, p[i]);
  }
  return ~crc;
}

// -----------------------------
// Helpers
// -----------------------------

static bool parseIp4(const char* s, Ip4& out) {
  if (!s) return false;
  int a, b, c, d;
  const int n = sscanf(s, "%d.%d.%d.%d", &a, &b, &c, &d);
  if (n != 4) return false;
  if (a < 0 || a > 255) return false;
  if (b < 0 || b > 255) return false;
  if (c < 0 || c > 255) return false;
  if (d < 0 || d > 255) return false;
  out = {(uint8_t)a, (uint8_t)b, (uint8_t)c, (uint8_t)d};
  return true;
}

static void formatIp4(const Ip4& ip, char* out, size_t outLen) {
  if (!out || outLen == 0) return;
  snprintf(out, outLen, "%u.%u.%u.%u", ip.a, ip.b, ip.c, ip.d);
}

static bool parseMac(const char* s, uint8_t out[6]) {
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

  // defaults seguros
  strncpy(c.mqtt.clientId, c.mqtt.deviceId, sizeof(c.mqtt.clientId) - 1);
  c.mqtt.clientId[sizeof(c.mqtt.clientId) - 1] = '\0';

  c.mqtt.username[0] = '\0';
  c.mqtt.apikey[0] = '\0';
}

const AppConfig& get() { return g_cfg; }
void set(const AppConfig& cfg) { g_cfg = cfg; }

// -----------------------------
// Load / Save vía registry TLV
// -----------------------------

bool loadOrDefault() {
  applyDefaults(g_cfg);

  auto& reg = app::nvm::registry();
  (void)reg.begin();

  if (!reg.isValid()) {
    // Registry no inicializado -> defaults.
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

  // mantener clientId alineado por defecto si está vacío
  if (g_cfg.mqtt.clientId[0] == '\0') {
    strncpy(g_cfg.mqtt.clientId, g_cfg.mqtt.deviceId, sizeof(g_cfg.mqtt.clientId) - 1);
    g_cfg.mqtt.clientId[sizeof(g_cfg.mqtt.clientId) - 1] = '\0';
  }

  if (reg.getBool(KEY_NET_DHCP, b)) {
    g_cfg.net.useDhcp = b;
  }

  if (reg.getStr(KEY_NET_IP, tmp, sizeof(tmp))) {
    (void)parseIp4(tmp, g_cfg.net.ip);
  }
  if (reg.getStr(KEY_NET_GW, tmp, sizeof(tmp))) {
    (void)parseIp4(tmp, g_cfg.net.gateway);
  }
  if (reg.getStr(KEY_NET_SUBNET, tmp, sizeof(tmp))) {
    (void)parseIp4(tmp, g_cfg.net.subnet);
  }
  if (reg.getStr(KEY_NET_DNS, tmp, sizeof(tmp))) {
    (void)parseIp4(tmp, g_cfg.net.dns);
  }
  if (reg.getStr(KEY_NET_MAC, tmp, sizeof(tmp))) {
    (void)parseMac(tmp, g_cfg.net.mac);
  }

  return true;
}

bool save() {
  auto& reg = app::nvm::registry();
  (void)reg.begin();
  if (!reg.isValid()) {
    (void)reg.format();
  }

  if (!reg.beginEdit()) return false;

  (void)reg.setU32(KEY_BACKEND_MODE, (g_cfg.backendMode == BACKEND_HA) ? 1u : 0u);
  (void)reg.setStr(KEY_MQTT_HOST, g_cfg.mqtt.host);
  (void)reg.setU32(KEY_MQTT_PORT, (uint32_t)g_cfg.mqtt.port);
  (void)reg.setStr(KEY_DEVICE_ID, g_cfg.mqtt.deviceId);

  (void)reg.setBool(KEY_NET_DHCP, g_cfg.net.useDhcp);

  uint32_t ip = ((uint32_t)g_cfg.net.ip.a)      | ((uint32_t)g_cfg.net.ip.b << 8)      | ((uint32_t)g_cfg.net.ip.c << 16)      | ((uint32_t)g_cfg.net.ip.d << 24);
  uint32_t gw = ((uint32_t)g_cfg.net.gateway.a) | ((uint32_t)g_cfg.net.gateway.b << 8) | ((uint32_t)g_cfg.net.gateway.c << 16) | ((uint32_t)g_cfg.net.gateway.d << 24);
  uint32_t sn = ((uint32_t)g_cfg.net.subnet.a)  | ((uint32_t)g_cfg.net.subnet.b << 8)  | ((uint32_t)g_cfg.net.subnet.c << 16)  | ((uint32_t)g_cfg.net.subnet.d << 24);
  uint32_t dns= ((uint32_t)g_cfg.net.dns.a)     | ((uint32_t)g_cfg.net.dns.b << 8)     | ((uint32_t)g_cfg.net.dns.c << 16)     | ((uint32_t)g_cfg.net.dns.d << 24);

  char macStr[24];
  formatMac(g_cfg.net.mac, macStr, sizeof(macStr));

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
