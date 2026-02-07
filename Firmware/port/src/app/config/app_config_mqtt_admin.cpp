#include "app/config/app_config_mqtt_admin.h"

#include "app/config/app_config.h"
#include "hal/hal_mqtt.h"

#include <Arduino.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

namespace app::cfgadmin {

static constexpr const char* BASE_TOPIC = "ferduino";

static void publishRetained(const char* topic, const char* payload) {
  if (!topic || !payload) return;
  (void)hal::mqtt().publish(topic, (const uint8_t*)payload, strlen(payload), true);
}

static void publishNonRetained(const char* topic, const char* payload) {
  if (!topic || !payload) return;
  (void)hal::mqtt().publish(topic, (const uint8_t*)payload, strlen(payload), false);
}

static void buildTopics(const char* deviceId, char* cmdTopic, size_t cmdLen,
                        char* cfgTopic, size_t cfgLen,
                        char* ackTopic, size_t ackLen) {
  snprintf(cmdTopic, cmdLen, "%s/%s/cmd/config", BASE_TOPIC, deviceId);
  snprintf(cfgTopic, cfgLen, "%s/%s/cfg", BASE_TOPIC, deviceId);
  snprintf(ackTopic, ackLen, "%s/%s/cfg/ack", BASE_TOPIC, deviceId);
}

// --- parsing helpers ---
static bool findValuePtr(const char* json, const char* key, const char** outPtr) {
  if (!json || !key || !outPtr) return false;

  char needle[48];
  snprintf(needle, sizeof(needle), "\"%s\"", key);

  const char* p = strstr(json, needle);
  if (!p) return false;

  p = strchr(p, ':');
  if (!p) return false;
  p++;

  while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
  *outPtr = p;
  return true;
}

static bool parseJsonString(const char* json, const char* key, char* out, size_t outLen) {
  const char* p = nullptr;
  if (!findValuePtr(json, key, &p)) return false;

  if (*p != '\"') return false;
  p++;

  size_t i = 0;
  while (*p && *p != '\"' && i + 1 < outLen) out[i++] = *p++;
  out[i] = '\0';
  return true;
}

static bool parseJsonInt(const char* json, const char* key, int& out) {
  const char* p = nullptr;
  if (!findValuePtr(json, key, &p)) return false;
  out = atoi(p);
  return true;
}

static const char* backendToStr(app::cfg::BackendMode m) {
  return (m == app::cfg::BACKEND_HA) ? "ha" : "legacy";
}

static app::cfg::BackendMode strToBackend(const char* s, bool& ok) {
  ok = false;
  if (!s) return app::cfg::BACKEND_LEGACY;
  if (strcmp(s, "ha") == 0) { ok = true; return app::cfg::BACKEND_HA; }
  if (strcmp(s, "legacy") == 0) { ok = true; return app::cfg::BACKEND_LEGACY; }
  return app::cfg::BACKEND_LEGACY;
}

static void publishCurrentCfg(const char* deviceIdForTopics) {
  const auto& c = app::cfg::get();

  char cmdTopic[96], cfgTopic[96], ackTopic[96];
  buildTopics(deviceIdForTopics, cmdTopic, sizeof(cmdTopic), cfgTopic, sizeof(cfgTopic), ackTopic, sizeof(ackTopic));

  char msg[360];
  snprintf(msg, sizeof(msg),
           "{"
             "\"backend\":\"%s\","
             "\"mqtt_host\":\"%s\","
             "\"mqtt_port\":%u,"
             "\"device_id\":\"%s\""
           "}",
           backendToStr(c.backendMode),
           c.mqtt.host,
           (unsigned)c.mqtt.port,
           c.mqtt.deviceId);

  publishRetained(cfgTopic, msg);
}

bool handleConfigCommand(const char* topic, const uint8_t* payload, size_t len) {
  if (!topic || !payload || len == 0) return false;

  if (app::cfg::get().mqtt.deviceId[0] == '\0') {
    (void)app::cfg::loadOrDefault();
  }

  const auto& current = app::cfg::get();

  char cmdTopic[96], cfgTopic[96], ackTopic[96];
  buildTopics(current.mqtt.deviceId, cmdTopic, sizeof(cmdTopic), cfgTopic, sizeof(cfgTopic), ackTopic, sizeof(ackTopic));

  if (strcmp(topic, cmdTopic) != 0) return false;

  char buf[220];
  const size_t n = (len >= sizeof(buf)) ? (sizeof(buf) - 1) : len;
  memcpy(buf, payload, n);
  buf[n] = '\0';

  if (strcmp(buf, "get") == 0 || strcmp(buf, "GET") == 0 || strcmp(buf, "?") == 0) {
    publishCurrentCfg(current.mqtt.deviceId);
    publishNonRetained(ackTopic, "{\"ok\":true,\"op\":\"get\"}");
    return true;
  }

  app::cfg::AppConfig next = current;

  bool changedMqtt = false;
  bool changedDeviceId = false;
  bool changedBackend = false;

  char tmp[80];

  if (parseJsonString(buf, "mqtt_host", tmp, sizeof(tmp))) {
    if (strncmp(next.mqtt.host, tmp, sizeof(next.mqtt.host)) != 0) {
      strncpy(next.mqtt.host, tmp, sizeof(next.mqtt.host) - 1);
      next.mqtt.host[sizeof(next.mqtt.host) - 1] = '\0';
      changedMqtt = true;
    }
  }

  int port = 0;
  if (parseJsonInt(buf, "mqtt_port", port)) {
    if (port > 0 && port < 65536 && next.mqtt.port != (uint16_t)port) {
      next.mqtt.port = (uint16_t)port;
      changedMqtt = true;
    }
  }

  if (parseJsonString(buf, "device_id", tmp, sizeof(tmp))) {
    if (tmp[0] != '\0' && strncmp(next.mqtt.deviceId, tmp, sizeof(next.mqtt.deviceId)) != 0) {
      strncpy(next.mqtt.deviceId, tmp, sizeof(next.mqtt.deviceId) - 1);
      next.mqtt.deviceId[sizeof(next.mqtt.deviceId) - 1] = '\0';
      changedDeviceId = true;
      changedMqtt = true; // clientId
    }
  }

  if (parseJsonString(buf, "backend", tmp, sizeof(tmp))) {
    bool ok = false;
    const app::cfg::BackendMode bm = strToBackend(tmp, ok);
    if (ok && next.backendMode != bm) {
      next.backendMode = bm;
      changedBackend = true;
    }
  }

  if (next.mqtt.host[0] == '\0' || next.mqtt.port == 0 || next.mqtt.deviceId[0] == '\0') {
    publishNonRetained(ackTopic, "{\"ok\":false,\"op\":\"set\",\"error\":\"invalid_config\"}");
    return true;
  }

  // Aplicar + guardar
  app::cfg::set(next);
  (void)app::cfg::save();

  // Publica cfg actual (retained) usando el *deviceId NUEVO* (si cambió, el topic cambia)
  publishCurrentCfg(next.mqtt.deviceId);

  // ACK (en el topic de ACK del deviceId anterior, para que el que envió el comando lo vea seguro)
  // + indicador reboot_required si cambió broker/clientId/backend (para reinit limpio)
  const bool rebootRequired = changedMqtt || changedBackend || changedDeviceId;

  char ack[220];
  snprintf(ack, sizeof(ack),
           "{"
             "\"ok\":true,"
             "\"op\":\"set\","
             "\"reboot_required\":%s"
           "}",
           rebootRequired ? "true" : "false");

  publishNonRetained(ackTopic, ack);
  return true;
}

} // namespace app::cfgadmin
