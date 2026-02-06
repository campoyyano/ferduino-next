#include "app/comms/ha/ha_legacy_bridge.h"

#include "hal/hal_mqtt.h"

#include <Arduino.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef FERDUINO_DEVICE_ID
  #error "FERDUINO_DEVICE_ID must be defined in platformio.ini build_flags"
#endif

namespace app::ha {

// Defaults (B2): constantes. En B3 se moverán a EEPROM/config runtime.
static constexpr const char* BASE_TOPIC = "ferduino";

static bool findValuePtr(const char* json, const char* key, const char** outPtr) {
  if (!json || !key || !outPtr) return false;

  // Buscamos el patrón: "key"
  char needle[48];
  snprintf(needle, sizeof(needle), "\"%s\"", key);

  const char* p = strstr(json, needle);
  if (!p) return false;

  // Buscar ':' después de la key
  p = strchr(p, ':');
  if (!p) return false;
  p++; // después de ':'

  // saltar espacios
  while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;

  *outPtr = p;
  return true;
}

static bool parseFloatKey(const char* json, const char* key, float& out) {
  const char* p = nullptr;
  if (!findValuePtr(json, key, &p)) return false;

  // Manejo simple de valores numéricos. (Legacy usa números sin comillas.)
  out = (float)atof(p);
  return true;
}

static bool parseIntKey(const char* json, const char* key, int& out) {
  const char* p = nullptr;
  if (!findValuePtr(json, key, &p)) return false;

  out = atoi(p);
  return true;
}

static void publishHaState(const char* payload) {
  if (!payload) return;

  char topic[96];
  snprintf(topic, sizeof(topic), "%s/%s/state", BASE_TOPIC, FERDUINO_DEVICE_ID);

  (void)hal::mqtt().publish(topic, (const uint8_t*)payload, strlen(payload), false);
}

void bridgeFromLegacyHomeJson(const char* legacyJson) {
  if (!legacyJson) return;

  // Defaults (si falta key, queda 0)
  float twater=0, theatsink=0, tamb=0;
  float waterph=0, reactorph=0, orp=0, dens=0;

  int wLedW=0, bLedW=0, rbLedW=0, redLedW=0, uvLedW=0;
  int outlet[10] = {0}; // 1..9
  int running=0;

  (void)parseFloatKey(legacyJson, "twater", twater);
  (void)parseFloatKey(legacyJson, "theatsink", theatsink);
  (void)parseFloatKey(legacyJson, "tamb", tamb);

  (void)parseFloatKey(legacyJson, "waterph", waterph);
  (void)parseFloatKey(legacyJson, "reactorph", reactorph);
  (void)parseFloatKey(legacyJson, "orp", orp);
  (void)parseFloatKey(legacyJson, "dens", dens);

  (void)parseIntKey(legacyJson, "wLedW", wLedW);
  (void)parseIntKey(legacyJson, "bLedW", bLedW);
  (void)parseIntKey(legacyJson, "rbLedW", rbLedW);
  (void)parseIntKey(legacyJson, "redLedW", redLedW);
  (void)parseIntKey(legacyJson, "uvLedW", uvLedW);

  (void)parseIntKey(legacyJson, "running", running);

  // outlets
  (void)parseIntKey(legacyJson, "outlet1", outlet[1]);
  (void)parseIntKey(legacyJson, "outlet2", outlet[2]);
  (void)parseIntKey(legacyJson, "outlet3", outlet[3]);
  (void)parseIntKey(legacyJson, "outlet4", outlet[4]);
  (void)parseIntKey(legacyJson, "outlet5", outlet[5]);
  (void)parseIntKey(legacyJson, "outlet6", outlet[6]);
  (void)parseIntKey(legacyJson, "outlet7", outlet[7]);
  (void)parseIntKey(legacyJson, "outlet8", outlet[8]);
  (void)parseIntKey(legacyJson, "outlet9", outlet[9]);

  // Construir HA state JSON (snake_case)
  // Nota: usaremos (double) para snprintf con %f (AVR).
  char ha[560];
  snprintf(ha, sizeof(ha),
           "{"
             "\"water_temperature\":%.2f,"
             "\"heatsink_temperature\":%.2f,"
             "\"ambient_temperature\":%.2f,"
             "\"water_ph\":%.2f,"
             "\"reactor_ph\":%.2f,"
             "\"orp\":%.2f,"
             "\"salinity\":%.3f,"
             "\"led_white_power\":%d,"
             "\"led_blue_power\":%d,"
             "\"led_royal_blue_power\":%d,"
             "\"led_red_power\":%d,"
             "\"led_uv_power\":%d,"
             "\"outlet_1\":%d,"
             "\"outlet_2\":%d,"
             "\"outlet_3\":%d,"
             "\"outlet_4\":%d,"
             "\"outlet_5\":%d,"
             "\"outlet_6\":%d,"
             "\"outlet_7\":%d,"
             "\"outlet_8\":%d,"
             "\"outlet_9\":%d,"
             "\"uptime\":%d"
           "}",
           (double)twater,
           (double)theatsink,
           (double)tamb,
           (double)waterph,
           (double)reactorph,
           (double)orp,
           (double)dens,
           wLedW, bLedW, rbLedW, redLedW, uvLedW,
           outlet[1], outlet[2], outlet[3], outlet[4], outlet[5], outlet[6], outlet[7], outlet[8], outlet[9],
           running);

  publishHaState(ha);
}

} // namespace app::ha
