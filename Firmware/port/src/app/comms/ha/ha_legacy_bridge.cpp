#include "app/comms/ha/ha_legacy_bridge.h"

#include "app/outlets/app_outlets.h"
#include "hal/hal_mqtt.h"

#include <Arduino.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef FERDUINO_DEVICE_ID
  #error "FERDUINO_DEVICE_ID must be defined in platformio.ini build_flags"
#endif

namespace app::ha {

// Defaults (B2/B3): constantes. En B3 se moverán a EEPROM/config runtime.
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
  p++;

  // Saltar espacios
  while (*p == ' ' || *p == '\t') p++;

  *outPtr = p;
  return true;
}

static float getFloatOr(const char* json, const char* key, float def) {
  const char* p = nullptr;
  if (!findValuePtr(json, key, &p)) return def;
  return (float)atof(p);
}

static int getIntOr(const char* json, const char* key, int def) {
  const char* p = nullptr;
  if (!findValuePtr(json, key, &p)) return def;
  return atoi(p);
}

static void publishHaState(const char* msg) {
  char topic[96];
  snprintf(topic, sizeof(topic), "%s/%s/state", BASE_TOPIC, FERDUINO_DEVICE_ID);
  (void)hal::mqtt().publish(topic, (const uint8_t*)msg, strlen(msg), false);
}

void publishHaStateFromLegacyHomeJson(const char* legacyJson) {
  if (!legacyJson) return;

  // Parse values (legacy keys)
  const float twater    = getFloatOr(legacyJson, "tempA", 0.0f);
  const float theatsink = getFloatOr(legacyJson, "tempH", 0.0f);
  const float tamb      = getFloatOr(legacyJson, "tempC", 0.0f);

  const float waterph   = getFloatOr(legacyJson, "phA", 0.0f);
  const float reactorph = getFloatOr(legacyJson, "phR", 0.0f);
  const float orp       = getFloatOr(legacyJson, "orp", 0.0f);
  const float dens      = getFloatOr(legacyJson, "dens", 0.0f);

  const int wLedW   = getIntOr(legacyJson, "ledW", 0);
  const int bLedW   = getIntOr(legacyJson, "ledB", 0);
  const int rbLedW  = getIntOr(legacyJson, "ledRB", 0);
  const int redLedW = getIntOr(legacyJson, "ledR", 0);
  const int uvLedW  = getIntOr(legacyJson, "ledUV", 0);

  const int running = getIntOr(legacyJson, "uptime", 0);

  // Preferimos outlets desde el motor HA (RAM + registry) para reflejar comandos HA
  int outlet[10] = {0};
  for (int i = 1; i <= 9; i++) {
    outlet[i] = app::outlets::get((uint8_t)(i - 1)) ? 1 : 0;
  }

  // Construir HA state JSON (snake_case)
  char ha[620];
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
