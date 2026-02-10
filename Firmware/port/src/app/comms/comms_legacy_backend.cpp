#include "app/comms_backend.h"
#include "app/comms_mode.h"
#include "app/comms/ha/ha_legacy_bridge.h"

#include "app/config/app_config.h"
#include "app/config/app_config_mqtt_admin.h"
#include "app/scheduler/app_event_scheduler.h"

#include "hal/hal_mqtt.h"
#include "hal/hal_network.h"

#include <Arduino.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ==== Build flags obligatorios (legacy auth) ==== */

#ifndef FERDUINO_LEGACY_USERNAME
  #error "Define FERDUINO_LEGACY_USERNAME in platformio.ini build_flags"
#endif

#ifndef FERDUINO_LEGACY_APIKEY
  #error "Define FERDUINO_LEGACY_APIKEY in platformio.ini build_flags"
#endif

namespace app {

#define D(x) ((double)(x))

/* ==== helpers ==== */

static void makeTopic(char* out, size_t outLen, const char* suffix) {
  if (!out || outLen == 0) return;
  const int n = snprintf(out, outLen, "%s/%s/%s", FERDUINO_LEGACY_USERNAME, FERDUINO_LEGACY_APIKEY, suffix);
  if (n < 0 || (size_t)n >= outLen) {
    // Truncado: dejar cadena vacía para evitar subscribes/publishes inconsistentes.
    out[0] = '\0';
    Serial.println("[legacy] ERROR: topic truncated (increase buffers or shorten username/apikey)");
  }
}

static void publishOk(const char* pubTopic) {
  const char ok[] = "{\"response\":\"ok\"}";
  (void)hal::mqtt().publish(pubTopic, (const uint8_t*)ok, sizeof(ok) - 1, false);
}

static void publishJson(const char* pubTopic, const char* json, bool retained = false) {
  if (!pubTopic || pubTopic[0] == '\0') return;
  if (!json) return;
  (void)hal::mqtt().publish(pubTopic, (const uint8_t*)json, strlen(json), retained);
}

/* ==== backend ==== */

class CommsLegacyBackend final : public ICommsBackend {
public:
  void begin() override {
    (void)app::cfg::loadOrDefault();
    const auto& appcfg = app::cfg::get();

    hal::MqttConfig cfg;
    cfg.host = appcfg.mqtt.host;
    cfg.port = appcfg.mqtt.port;

    // Política: usar SIEMPRE deviceId como MQTT clientId para evitar dualidad LWT/status.
    cfg.clientId = appcfg.mqtt.deviceId;
    cfg.keepAliveSec = 30;

    (void)hal::mqtt().begin(cfg);
    hal::mqtt().onMessage(&CommsLegacyBackend::onMqttMessageStatic);

    makeTopic(_subTopic, sizeof(_subTopic), "topic/command");
    makeTopic(_pubTopic, sizeof(_pubTopic), "topic/response");

    // admin config topic (siempre disponible aunque backend sea legacy)
    {
      const int n = snprintf(_cmdCfgTopic, sizeof(_cmdCfgTopic),
                             "ferduino/%s/cmd/config", appcfg.mqtt.deviceId);
      if (n < 0 || (size_t)n >= sizeof(_cmdCfgTopic)) {
        _cmdCfgTopic[0] = '\0';
        Serial.println("[legacy] ERROR: cmd config topic truncated");
      }
    }

    // C1.2: schedule cmd/config topics (namespace ferduino/<deviceId>/...)
    for (int i = 1; i <= 9; i++) {
      const int n1 = snprintf(_cmdScheduleTopic[i], sizeof(_cmdScheduleTopic[i]),
                              "ferduino/%s/cmd/schedule/%d", appcfg.mqtt.deviceId, i);
      if (n1 < 0 || (size_t)n1 >= sizeof(_cmdScheduleTopic[i])) {
        _cmdScheduleTopic[i][0] = '\0';
        Serial.println("[legacy] ERROR: cmd schedule topic truncated");
      }

      const int n2 = snprintf(_cfgScheduleTopic[i], sizeof(_cfgScheduleTopic[i]),
                              "ferduino/%s/cfg/schedule/%d", appcfg.mqtt.deviceId, i);
      if (n2 < 0 || (size_t)n2 >= sizeof(_cfgScheduleTopic[i])) {
        _cfgScheduleTopic[i][0] = '\0';
        Serial.println("[legacy] ERROR: cfg schedule topic truncated");
      }
    }

    {
      const int n3 = snprintf(_ackScheduleTopic, sizeof(_ackScheduleTopic),
                              "ferduino/%s/cfg/schedule/ack", appcfg.mqtt.deviceId);
      if (n3 < 0 || (size_t)n3 >= sizeof(_ackScheduleTopic)) {
        _ackScheduleTopic[0] = '\0';
        Serial.println("[legacy] ERROR: ack schedule topic truncated");
      }
    }

    _lastReconnectMs = 0;
  }

  void loop() override {
    hal::network().loop();
    hal::mqtt().loop();

    if (!hal::network().isInitialized())
      return;

    if (!hal::mqtt().connected()) {
      const uint32_t now = millis();
      if (now - _lastReconnectMs < 2000)
        return;

      _lastReconnectMs = now;

      if (hal::mqtt().connect() == hal::MqttError::Ok) {
        if (_subTopic[0] != '\0') {
          (void)hal::mqtt().subscribe(_subTopic);
        }

        // subscribe admin config
        if (_cmdCfgTopic[0] != '\0') {
          (void)hal::mqtt().subscribe(_cmdCfgTopic);
        }

        // C1.2: subscribe schedule commands
        for (int i = 1; i <= 9; i++) {
          if (_cmdScheduleTopic[i][0] != '\0') {
            (void)hal::mqtt().subscribe(_cmdScheduleTopic[i]);
          }
        }

        // C1.2: publicar retained de schedule en reconnect
        publishAllSchedulesRetained();

        // Online retained (útil para legacy; si molestase, se retira)
        const char online[] = "{\"online\":1}";
        publishJson(_pubTopic, online, true);
      }
    }
  }

  bool connected() const override {
    return hal::mqtt().connected();
  }

  bool publishStatus(const char* key,
                     const char* value,
                     bool retained=false) override {
    if (!key || !value) return false;
    if (_pubTopic[0] == '\0') return false;

    char msg[160];
    snprintf(msg, sizeof(msg), "{\"%s\":\"%s\"}", key, value);

    return hal::mqtt().publish(
      _pubTopic,
      (const uint8_t*)msg,
      strlen(msg),
      retained
    ) == hal::MqttError::Ok;
  }

  void onMqttMessage(const char* topic, const uint8_t* payload, size_t len) override {
    if (!topic || !payload || len == 0) return;

    // Admin config primero (si coincide topic)
    if (_cmdCfgTopic[0] != '\0' && app::cfgadmin::handleConfigCommand(topic, payload, len)) {
      return;
    }

    // C1.2: schedule cmd (JSON) en namespace ferduino/<deviceId>/cmd/schedule/N
    for (int i = 1; i <= 9; i++) {
      if (_cmdScheduleTopic[i][0] != '\0' && strcmp(topic, _cmdScheduleTopic[i]) == 0) {
        // Copia payload a buffer pequeño
        char json[96];
        const size_t n = (len >= sizeof(json)) ? (sizeof(json) - 1) : len;
        memcpy(json, payload, n);
        json[n] = '\0';

        handleScheduleCommand((uint8_t)(i - 1), json);
        return;
      }
    }

    if (_subTopic[0] == '\0') return;
    if (strcmp(topic, _subTopic) != 0) return;

    const size_t n = (len >= sizeof(_rxBuf)) ? (sizeof(_rxBuf) - 1) : len;
    memcpy(_rxBuf, payload, n);
    _rxBuf[n] = '\0';

    // Legacy original: tokens separados por '/'
    // Ej: "00/0" , "01/0", ...
    int argc = 0;
    char* p = _rxBuf;
    while (p && *p && argc < (int)MAX_TOKENS) {
      _argv[argc++] = p;
      char* slash = strchr(p, '/');
      if (!slash) break;
      *slash = '\0';
      p = slash + 1;
    }

    if (argc <= 0) return;

    const int id = atoi(_argv[0]);
    handleById(id, argc);
  }

  static void onMqttMessageStatic(const char* topic, const uint8_t* payload, size_t len) {
    instance().onMqttMessage(topic, payload, len);
  }

  static CommsLegacyBackend& instance() {
    static CommsLegacyBackend inst;
    return inst;
  }

private:
  // ========== ID 0 ==========
  void handleTemps_ID0(int argc) {
    const int mode = (argc >= 2) ? atoi(_argv[1]) : 0;
    if (mode == 0) {
      const float setTemp=0.0f, offTemp=0.0f, alarmTemp=0.0f;
      char msg[200];
      snprintf(msg, sizeof(msg),
               "{\"setTemp\":%.2f,\"offTemp\":%.2f,\"alarmTemp\":%.2f}",
               D(setTemp),
               D(offTemp),
               D(alarmTemp));
      publishJson(_pubTopic, msg, false);
      return;
    }
    publishOk(_pubTopic);
  }

  // ========== ID 1 ==========
  void handlePH_ID1(int argc) {
    (void)argc;
    publishOk(_pubTopic);
  }

  // ========== ID 2 ==========
  void handleORP_ID2(int argc) {
    (void)argc;
    publishOk(_pubTopic);
  }

  // ========== ID 3 ==========
  void handlePress_ID3(int argc) {
    (void)argc;
    publishOk(_pubTopic);
  }

  // ========== ID 4 ==========
  void handleLeds_ID4(int argc) {
    (void)argc;
    publishOk(_pubTopic);
  }

  // ========== ID 5 ==========
  void handleSockets_ID5(int argc) {
    (void)argc;
    publishOk(_pubTopic);
  }

  // ========== ID 6 ==========
  void handleDosing_ID6(int argc) {
    (void)argc;
    publishOk(_pubTopic);
  }

  // ========== ID 7 ==========
  void handleRunners_ID7(int argc) {
    (void)argc;
    publishOk(_pubTopic);
  }

  // ========== ID 8 ==========
  void handleOther_ID8(int argc) {
    (void)argc;
    publishOk(_pubTopic);
  }

  void handleById(int id, int argc) {
    switch (id) {
      case 0: handleTemps_ID0(argc); return;
      case 1: handlePH_ID1(argc); return;
      case 2: handleORP_ID2(argc); return;
      case 3: handlePress_ID3(argc); return;
      case 4: handleLeds_ID4(argc); return;
      case 5: handleSockets_ID5(argc); return;
      case 6: handleDosing_ID6(argc); return;
      case 7: handleRunners_ID7(argc); return;
      case 8: handleOther_ID8(argc); return;
      default:
        break;
    }

    char msg[96];
    snprintf(msg, sizeof(msg), "{\"error\":\"unknown_id\",\"id\":%d}", id);
    publishJson(_pubTopic, msg, false);
  }

private:
  static bool jsonFindString(const char* json, const char* key, char* out, size_t outLen) {
    if (!json || !key || !out || outLen == 0) return false;
    out[0] = '\0';

    char pat[24];
    snprintf(pat, sizeof(pat), "\"%s\"", key);

    const char* p = strstr(json, pat);
    if (!p) return false;
    p = strchr(p, ':');
    if (!p) return false;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    if (*p != '"') return false;
    p++;

    size_t i = 0;
    while (*p && *p != '"' && i + 1 < outLen) {
      out[i++] = *p++;
    }
    out[i] = '\0';
    return (*p == '"');
  }

  static bool jsonFindBool(const char* json, const char* key, bool& out) {
    if (!json || !key) return false;

    char pat[24];
    snprintf(pat, sizeof(pat), "\"%s\"", key);

    const char* p = strstr(json, pat);
    if (!p) return false;
    p = strchr(p, ':');
    if (!p) return false;
    p++;
    while (*p == ' ' || *p == '\t') p++;

    if (strncmp(p, "true", 4) == 0) {
      out = true;
      return true;
    }
    if (strncmp(p, "false", 5) == 0) {
      out = false;
      return true;
    }
    if (*p == '1') { out = true; return true; }
    if (*p == '0') { out = false; return true; }
    return false;
  }

  static bool parseHHMM(const char* s, app::scheduler::TimeHM& out) {
    if (!s) return false;
    if (!(s[0] >= '0' && s[0] <= '9')) return false;
    if (!(s[1] >= '0' && s[1] <= '9')) return false;
    if (s[2] != ':') return false;
    if (!(s[3] >= '0' && s[3] <= '9')) return false;
    if (!(s[4] >= '0' && s[4] <= '9')) return false;

    const uint8_t hh = (uint8_t)((s[0] - '0') * 10 + (s[1] - '0'));
    const uint8_t mm = (uint8_t)((s[3] - '0') * 10 + (s[4] - '0'));
    if (hh > 23 || mm > 59) return false;

    out.hour = hh;
    out.minute = mm;
    return true;
  }

  void publishScheduleCfgRetained(uint8_t ch) {
    if (ch >= 9) return;
    if (_cfgScheduleTopic[ch + 1][0] == '\0') return;

    const auto w = app::scheduler::events::window(ch);

    char msg[96];
    snprintf(msg, sizeof(msg),
             "{\"ch\":%u,\"enabled\":%u,\"on\":%u,\"off\":%u}",
             (unsigned)(ch + 1),
             (unsigned)(w.enabled ? 1 : 0),
             (unsigned)w.onMinute,
             (unsigned)w.offMinute);

    (void)hal::mqtt().publish(_cfgScheduleTopic[ch + 1], (const uint8_t*)msg, strlen(msg), true);
  }

  void publishAllSchedulesRetained() {
    for (uint8_t ch = 0; ch < 9; ++ch) {
      publishScheduleCfgRetained(ch);
    }
  }

  void publishScheduleAck(bool ok, uint8_t ch) {
    if (_ackScheduleTopic[0] == '\0') return;
    char msg[64];
    snprintf(msg, sizeof(msg),
             "{\"ok\":%s,\"ch\":%u}",
             ok ? "true" : "false",
             (unsigned)(ch + 1));
    (void)hal::mqtt().publish(_ackScheduleTopic, (const uint8_t*)msg, strlen(msg), false);
  }

  void handleScheduleCommand(uint8_t ch, const char* json) {
    if (!json) return;

    const auto cur = app::scheduler::events::window(ch);

    bool hasOn = false;
    bool hasOff = false;
    bool hasEnabled = false;

    app::scheduler::TimeHM onHm{};
    app::scheduler::TimeHM offHm{};

    char tmp[8];

    if (jsonFindString(json, "on", tmp, sizeof(tmp))) {
      hasOn = parseHHMM(tmp, onHm);
    }
    if (jsonFindString(json, "off", tmp, sizeof(tmp))) {
      hasOff = parseHHMM(tmp, offHm);
    }

    bool enabled = cur.enabled;
    if (jsonFindBool(json, "enabled", enabled)) {
      hasEnabled = true;
    }

    if ((hasOn || hasOff) && !hasEnabled) {
      enabled = true;
    }

    if (!hasOn) {
      onHm.hour = (uint8_t)(cur.onMinute / 60u);
      onHm.minute = (uint8_t)(cur.onMinute % 60u);
    }
    if (!hasOff) {
      offHm.hour = (uint8_t)(cur.offMinute / 60u);
      offHm.minute = (uint8_t)(cur.offMinute % 60u);
    }

    const bool ok = app::scheduler::events::setWindow(ch, onHm, offHm, enabled);
    if (ok) {
      publishScheduleCfgRetained(ch);
    }
    publishScheduleAck(ok, ch);
  }

  static constexpr size_t RX_BUF_SIZE = 256;
  static constexpr size_t MAX_TOKENS  = 32;

  char _subTopic[128] = {0};
  char _pubTopic[128] = {0};
  char _cmdCfgTopic[96] = {0};

  // C1.2 schedule cmd + cfg topics (índice 1..9)
  char _cmdScheduleTopic[10][96] = {{0}};
  char _cfgScheduleTopic[10][96] = {{0}};
  char _ackScheduleTopic[96] = {0};

  uint32_t _lastReconnectMs = 0;

  char _rxBuf[RX_BUF_SIZE] = {0};
  char* _argv[MAX_TOKENS]  = {0};
};

/* ==== Factory hook ==== */

ICommsBackend& comms_legacy_singleton() {
  return CommsLegacyBackend::instance();
}

} // namespace app
