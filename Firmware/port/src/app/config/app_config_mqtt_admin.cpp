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

// -----------------------------
// JSON helpers (minimalistas, sin ArduinoJson)
// -----------------------------
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

static bool parseJsonBool(const char* json, const char* key, bool& out) {
  const char* p = nullptr;
  if (!findValuePtr(json, key, &p)) return false;

  if (strncmp(p, "true", 4) == 0)  { out = true;  return true; }
  if (strncmp(p, "false", 5) == 0) { out = false; return true; }

  // también aceptamos 0/1
  if (*p == '0') { out = false; return true; }
  if (*p == '1') { out = true;  return true; }

  return false;
}

static bool parseIp4(const char* s, app::cfg::Ip4& out) {
  if (!s) return false;
  unsigned int a=0,b=0,c=0,d=0;
  const int n = sscanf(s, "%u.%u.%u.%u", &a, &b, &c, &d);
  if (n != 4) return false;
  if (a > 255 || b > 255 || c > 255 || d > 255) return false;
  out = {(uint8_t)a,(uint8_t)b,(uint8_t)c,(uint8_t)d};
  return true;
}

static bool parseJsonIp(const char* json, const char* key, app::cfg::Ip4& out) {
  char tmp[32];
  if (!parseJsonString(json, key, tmp, sizeof(tmp))) return false;
  return parseIp4(tmp, out);
}

static bool parseMac(const char* s, uint8_t out[6]) {
  // Formato: "02:FD:00:00:00:01"
  if (!s) return false;
  unsigned int b[6] = {0};
  const int n = sscanf(s, "%2x:%2x:%2x:%2x:%2x:%2x",
                       &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]);
  if (n != 6) return false;
  for (int i = 0; i < 6; ++i) out[i] = (uint8_t)b[i];
  return true;
}

static void formatIp(const app::cfg::Ip4& ip, char* out, size_t outLen) {
  if (!out || outLen == 0) return;
  snprintf(out, outLen, "%u.%u.%u.%u", (unsigned)ip.a, (unsigned)ip.b, (unsigned)ip.c, (unsigned)ip.d);
}

static void formatMac(const uint8_t mac[6], char* out, size_t outLen) {
  if (!out || outLen == 0) return;
  snprintf(out, outLen, "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static const char* backendToStr(app::cfg::BackendMode m) {
  return (m == app::cfg::BACKEND_HA) ? "ha" : "legacy";
}

static app::cfg::BackendMode strToBackend(const char* s, bool& ok) {
  ok = false;
  if (!s) return app::cfg::BACKEND_LEGACY;
  if (strcmp(s, "ha") == 0)     { ok = true; return app::cfg::BACKEND_HA; }
  if (strcmp(s, "legacy") == 0) { ok = true; return app::cfg::BACKEND_LEGACY; }
  return app::cfg::BACKEND_LEGACY;
}

static void publishCurrentCfg(const char* deviceIdForTopics) {
  const auto& c = app::cfg::get();

  char cmdTopic[96], cfgTopic[96], ackTopic[96];
  buildTopics(deviceIdForTopics, cmdTopic, sizeof(cmdTopic), cfgTopic, sizeof(cfgTopic), ackTopic, sizeof(ackTopic));

  char ip[24], gw[24], sn[24], dns[24], mac[24];
  formatIp(c.net.ip, ip, sizeof(ip));
  formatIp(c.net.gateway, gw, sizeof(gw));
  formatIp(c.net.subnet, sn, sizeof(sn));
  formatIp(c.net.dns, dns, sizeof(dns));
  formatMac(c.net.mac, mac, sizeof(mac));

  char msg[520];
  snprintf(msg, sizeof(msg),
           "{"
             "\"backend\":\"%s\","
             "\"mqtt_host\":\"%s\","
             "\"mqtt_port\":%u,"
             "\"device_id\":\"%s\","
             "\"net_dhcp\":%s,"
             "\"net_ip\":\"%s\","
             "\"net_gw\":\"%s\","
             "\"net_subnet\":\"%s\","
             "\"net_dns\":\"%s\","
             "\"net_mac\":\"%s\""
           "}",
           backendToStr(c.backendMode),
           c.mqtt.host,
           (unsigned)c.mqtt.port,
           c.mqtt.deviceId,
           c.net.useDhcp ? "true" : "false",
           ip, gw, sn, dns, mac);

  publishRetained(cfgTopic, msg);
}

bool handleConfigCommand(const char* topic, const uint8_t* payload, size_t len) {
  if (!topic || !payload || len == 0) return false;

  // Asegura que hay config cargada
  if (app::cfg::get().mqtt.deviceId[0] == '\0') {
    (void)app::cfg::loadOrDefault();
  }

  const auto& current = app::cfg::get();

  char cmdTopic[96], cfgTopic[96], ackTopic[96];
  buildTopics(current.mqtt.deviceId, cmdTopic, sizeof(cmdTopic), cfgTopic, sizeof(cfgTopic), ackTopic, sizeof(ackTopic));

  // Este handler solo procesa su topic
  if (strcmp(topic, cmdTopic) != 0) return false;

  // Copiamos payload a buffer
  char buf[420];
  const size_t n = (len >= sizeof(buf)) ? (sizeof(buf) - 1) : len;
  memcpy(buf, payload, n);
  buf[n] = '\0';

  // Operación GET simple
  if (strcmp(buf, "get") == 0 || strcmp(buf, "GET") == 0 || strcmp(buf, "?") == 0) {
    publishCurrentCfg(current.mqtt.deviceId);
    publishNonRetained(ackTopic, "{\"ok\":true,\"op\":\"get\"}");
    return true;
  }

  // SET parcial
  app::cfg::AppConfig next = current;

  bool changedMqtt = false;
  bool changedDeviceId = false;
  bool changedBackend = false;
  bool changedNet = false;

  char tmp[96];

  // mqtt_host
  if (parseJsonString(buf, "mqtt_host", tmp, sizeof(tmp))) {
    if (strncmp(next.mqtt.host, tmp, sizeof(next.mqtt.host)) != 0) {
      strncpy(next.mqtt.host, tmp, sizeof(next.mqtt.host) - 1);
      next.mqtt.host[sizeof(next.mqtt.host) - 1] = '\0';
      changedMqtt = true;
    }
  }

  // mqtt_port
  int port = 0;
  if (parseJsonInt(buf, "mqtt_port", port)) {
    if (port > 0 && port < 65536 && next.mqtt.port != (uint16_t)port) {
      next.mqtt.port = (uint16_t)port;
      changedMqtt = true;
    }
  }

  // device_id
  if (parseJsonString(buf, "device_id", tmp, sizeof(tmp))) {
    if (tmp[0] != '\0' && strncmp(next.mqtt.deviceId, tmp, sizeof(next.mqtt.deviceId)) != 0) {
      strncpy(next.mqtt.deviceId, tmp, sizeof(next.mqtt.deviceId) - 1);
      next.mqtt.deviceId[sizeof(next.mqtt.deviceId) - 1] = '\0';
      changedDeviceId = true;
      changedMqtt = true; // cambia clientId/topic base
    }
  }

  // backend
  if (parseJsonString(buf, "backend", tmp, sizeof(tmp))) {
    bool ok = false;
    const app::cfg::BackendMode bm = strToBackend(tmp, ok);
    if (ok && next.backendMode != bm) {
      next.backendMode = bm;
      changedBackend = true;
    }
  }

  // net_dhcp
  bool dhcp = false;
  if (parseJsonBool(buf, "net_dhcp", dhcp)) {
    if (next.net.useDhcp != dhcp) {
      next.net.useDhcp = dhcp;
      changedNet = true;
    }
  }

  // IPs (solo si vienen en el JSON)
  app::cfg::Ip4 ip{};
  if (parseJsonIp(buf, "net_ip", ip)) {
    if (memcmp(&next.net.ip, &ip, sizeof(ip)) != 0) {
      next.net.ip = ip;
      changedNet = true;
    }
  }

  app::cfg::Ip4 gw{};
  if (parseJsonIp(buf, "net_gw", gw)) {
    if (memcmp(&next.net.gateway, &gw, sizeof(gw)) != 0) {
      next.net.gateway = gw;
      changedNet = true;
    }
  }

  app::cfg::Ip4 sn{};
  if (parseJsonIp(buf, "net_subnet", sn)) {
    if (memcmp(&next.net.subnet, &sn, sizeof(sn)) != 0) {
      next.net.subnet = sn;
      changedNet = true;
    }
  }

  app::cfg::Ip4 dns{};
  if (parseJsonIp(buf, "net_dns", dns)) {
    if (memcmp(&next.net.dns, &dns, sizeof(dns)) != 0) {
      next.net.dns = dns;
      changedNet = true;
    }
  }

  // net_mac
  if (parseJsonString(buf, "net_mac", tmp, sizeof(tmp))) {
    uint8_t mac[6];
    if (parseMac(tmp, mac)) {
      if (memcmp(next.net.mac, mac, 6) != 0) {
        memcpy(next.net.mac, mac, 6);
        changedNet = true;
      }
    } else {
      publishNonRetained(ackTopic, "{\"ok\":false,\"op\":\"set\",\"error\":\"invalid_mac\"}");
      return true;
    }
  }

  // Validación mínima
  if (next.mqtt.host[0] == '\0' || next.mqtt.port == 0 || next.mqtt.deviceId[0] == '\0') {
    publishNonRetained(ackTopic, "{\"ok\":false,\"op\":\"set\",\"error\":\"invalid_config\"}");
    return true;
  }

  // Aplicar + persistir (registry TLV)
  app::cfg::set(next);
  (void)app::cfg::save();

  // Publica cfg actual (retained) usando el deviceId NUEVO si cambió
  publishCurrentCfg(next.mqtt.deviceId);

  // ACK en el ackTopic del deviceId anterior (el emisor lo ve seguro)
  const bool rebootRequired = changedMqtt || changedBackend || changedDeviceId || changedNet;

  char ack[240];
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
