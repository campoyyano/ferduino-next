#include "app/config/app_config.h"

#include <EEPROM.h>
#include <string.h>

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

static constexpr uint32_t CFG_MAGIC = 0x584E4446UL; // 'F''D''N''X' little-endian
static constexpr int EEPROM_ADDR = 0;

namespace app::cfg {

static AppConfig g_cfg{};

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

void applyDefaults(AppConfig& c) {
  memset(&c, 0, sizeof(c));
  c.magic = CFG_MAGIC;
  c.version = 1;
  c.size = (uint16_t)sizeof(AppConfig);
  c.crc32 = 0;

#if (FERDUINO_COMMS_MODE == FERDUINO_COMMS_HA)
  c.backendMode = BACKEND_HA;
#else
  c.backendMode = BACKEND_LEGACY;
#endif

  c.net.mac[0] = 0x02;
  c.net.mac[1] = 0xFD;
  c.net.mac[2] = 0x00;
  c.net.mac[3] = 0x00;
  c.net.mac[4] = 0x00;
  c.net.mac[5] = 0x01;

  c.net.useDhcp = true;
  c.net.ip      = {0,0,0,0};
  c.net.gateway = {0,0,0,0};
  c.net.subnet  = {255,255,255,0};
  c.net.dns     = {0,0,0,0};

  strncpy(c.mqtt.host, MQTT_BROKER_HOST, sizeof(c.mqtt.host) - 1);
  c.mqtt.host[sizeof(c.mqtt.host) - 1] = '\0';
  c.mqtt.port = (uint16_t)MQTT_BROKER_PORT;

  strncpy(c.mqtt.deviceId, FERDUINO_DEVICE_ID, sizeof(c.mqtt.deviceId) - 1);
  c.mqtt.deviceId[sizeof(c.mqtt.deviceId) - 1] = '\0';
}

const AppConfig& get() { return g_cfg; }

void set(const AppConfig& cfg) {
  g_cfg = cfg;
}

static bool isValid(const AppConfig& c) {
  if (c.magic != CFG_MAGIC) return false;
  if (c.version != 1) return false;
  if (c.size != sizeof(AppConfig)) return false;

  AppConfig tmp = c;
  const uint32_t stored = tmp.crc32;
  tmp.crc32 = 0;
  const uint32_t calc = computeCrc32(&tmp, sizeof(AppConfig));
  return stored == calc;
}

bool loadOrDefault() {
  AppConfig tmp;
  EEPROM.get(EEPROM_ADDR, tmp);

  if (isValid(tmp)) {
    g_cfg = tmp;
    return true;
  }

  applyDefaults(g_cfg);
  return false;
}

bool save() {
  AppConfig tmp = g_cfg;
  tmp.magic = CFG_MAGIC;
  tmp.version = 1;
  tmp.size = (uint16_t)sizeof(AppConfig);

  tmp.crc32 = 0;
  tmp.crc32 = computeCrc32(&tmp, sizeof(AppConfig));

  EEPROM.put(EEPROM_ADDR, tmp);
  g_cfg = tmp;
  return true;
}

void factoryReset() {
  applyDefaults(g_cfg);
  (void)save();
}

} // namespace app::cfg
