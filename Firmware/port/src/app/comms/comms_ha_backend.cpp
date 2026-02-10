#include "app/comms/comms_ha_backend.h"

#include "app/config/app_config.h"
#include "app/config/app_config_mqtt_admin.h"

#include "app/comms/ha/ha_discovery.h"
#include "app/outlets/app_outlets.h"
#include "app/scheduler/app_scheduler.h"
#include "app/scheduler/app_event_scheduler.h"

#include "hal/hal_mqtt.h"
#include "hal/hal_network.h"

#include <Arduino.h>
#include <string.h>
#include <stdio.h>

namespace app {

static constexpr const char* BASE_TOPIC = "ferduino";

class CommsHABackend final : public ICommsBackend {
public:
  void begin() override {
    // No hay setup() global todavía -> garantizamos cfg en RAM
    (void)app::cfg::loadOrDefault();

    const auto& cfg = app::cfg::get();
    const char* deviceId = cfg.mqtt.deviceId;

    hal::MqttConfig mcfg;
    mcfg.host = cfg.mqtt.host;
    mcfg.port = cfg.mqtt.port;
    mcfg.clientId = deviceId;
    mcfg.keepAliveSec = 30;

    (void)hal::mqtt().begin(mcfg);
    hal::mqtt().onMessage(&CommsHABackend::onMqttMessageStatic);

    snprintf(_stateTopic, sizeof(_stateTopic), "%s/%s/state", BASE_TOPIC, deviceId);
    snprintf(_availTopic, sizeof(_availTopic), "%s/%s/availability", BASE_TOPIC, deviceId);

    for (int i = 1; i <= 9; i++) {
      snprintf(_cmdOutletTopic[i], sizeof(_cmdOutletTopic[i]),
               "%s/%s/cmd/outlet/%d", BASE_TOPIC, deviceId, i);
    }

    // C1.2: schedule cmd/config topics
    for (int i = 1; i <= 9; i++) {
      snprintf(_cmdScheduleTopic[i], sizeof(_cmdScheduleTopic[i]),
               "%s/%s/cmd/schedule/%d", BASE_TOPIC, deviceId, i);
      snprintf(_cfgScheduleTopic[i], sizeof(_cfgScheduleTopic[i]),
               "%s/%s/cfg/schedule/%d", BASE_TOPIC, deviceId, i);
    }
    snprintf(_ackScheduleTopic, sizeof(_ackScheduleTopic),
             "%s/%s/cfg/schedule/ack", BASE_TOPIC, deviceId);

    // Config admin topic
    snprintf(_cmdCfgTopic, sizeof(_cmdCfgTopic), "%s/%s/cmd/config", BASE_TOPIC, deviceId);

    _lastReconnectMs = millis();
    _lastStateMs = millis();
    _discoveryPublished = false;
    _subscribed = false;
  }

  void loop() override {
    const uint32_t now = millis();

    if (!hal::mqtt().connected()) {
      if (now - _lastReconnectMs > 3000) {
        _lastReconnectMs = now;
        (void)hal::mqtt().connect();
      }
      _subscribed = false;
      return;
    }

    // Subscribe una vez al conectar
    if (!_subscribed) {
      _subscribed = true;

      // Config admin
      (void)hal::mqtt().subscribe(_cmdCfgTopic);

      // Outlets
      for (int i = 1; i <= 9; i++) {
        (void)hal::mqtt().subscribe(_cmdOutletTopic[i]);
      }

      // C1.2: schedule commands
      for (int i = 1; i <= 9; i++) {
        (void)hal::mqtt().subscribe(_cmdScheduleTopic[i]);
      }

      // Discovery
      if (!_discoveryPublished) {
        app::ha::publishDiscoveryMinimal();
        _discoveryPublished = true;
      }

      publishAvailability(true);
      publishStatePlaceholder();

      // C1.2: publicar config schedule retenida para que HA/cliente lo vea tras reconnect
      publishAllSchedulesRetained();
    }

    // publish periódico de estado (placeholder)
    if (now - _lastStateMs > 5000) {
      _lastStateMs = now;
      publishStatePlaceholder();
    }
  }

  bool connected() const override {
    return hal::mqtt().connected();
  }

  bool publishStatus(const char* key, const char* value, bool retained=false) override {
    if (!key || !value) return false;

    char t[128];
    snprintf(t, sizeof(t), "%s/%s/status/%s", BASE_TOPIC, app::cfg::get().mqtt.deviceId, key);

    return hal::mqtt().publish(
      t,
      (const uint8_t*)value,
      strlen(value),
      retained
    ) == hal::MqttError::Ok;
  }

  void onMqttMessage(const char* topic, const uint8_t* payload, size_t len) override {
    if (!topic || !payload || len == 0) return;

    // Admin config primero
    if (app::cfgadmin::handleConfigCommand(topic, payload, len)) {
      return;
    }

    // Copia payload
    const size_t n = (len >= sizeof(_rx)) ? (sizeof(_rx) - 1) : len;
    memcpy(_rx, payload, n);
    _rx[n] = '\0';

    // C1.2: schedule cmd (JSON)
    for (int i = 1; i <= 9; i++) {
      if (strcmp(topic, _cmdScheduleTopic[i]) == 0) {
        handleScheduleCommand((uint8_t)(i - 1), _rx);
        return;
      }
    }

    // Outlets 1..9 (payload "0"/"1")
    for (int i = 1; i <= 9; i++) {
      if (strcmp(topic, _cmdOutletTopic[i]) == 0) {
        const uint8_t v = (_rx[0] == '1') ? 1 : 0;
        app::outlets::set((uint8_t)(i - 1), (v != 0));
        publishStatePlaceholder();
        return;
      }
    }
  }

  static void onMqttMessageStatic(const char* topic, const uint8_t* payload, size_t len) {
    instance().onMqttMessage(topic, payload, len);
  }

  static CommsHABackend& instance() {
    static CommsHABackend inst;
    return inst;
  }

private:
  static const char* sourceToStr(app::scheduler::TimeSource s) {
    return (s == app::scheduler::TimeSource::Rtc) ? "rtc" : "millis";
  }

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
    // formato "HH:MM"
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

  void publishAvailability(bool online) {
    const char* v = online ? "online" : "offline";
    (void)hal::mqtt().publish(_availTopic, (const uint8_t*)v, strlen(v), true);
  }

  void publishScheduleCfgRetained(uint8_t ch) {
    if (ch >= 9) return;
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
    char msg[64];
    snprintf(msg, sizeof(msg),
             "{\"ok\":%s,\"ch\":%u}",
             ok ? "true" : "false",
             (unsigned)(ch + 1));
    (void)hal::mqtt().publish(_ackScheduleTopic, (const uint8_t*)msg, strlen(msg), false);
  }

  void handleScheduleCommand(uint8_t ch, const char* json) {
    // Payload esperado: {"on":"HH:MM","off":"HH:MM","enabled":true}
    // Reglas:
    // - si viene on/off y NO viene enabled => enabled=true
    // - si falta on u off => mantener valor actual
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

    // si llegan horarios pero no enabled -> enabled=true
    if ((hasOn || hasOff) && !hasEnabled) {
      enabled = true;
    }

    // mantener valores actuales si no llega campo
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

  void publishStatePlaceholder() {
    // Estado mínimo coherente con discovery B3.x
    const float water_temperature    = 0.0f;
    const float heatsink_temperature = 0.0f;
    const float ambient_temperature  = 0.0f;

    const float water_ph   = 0.0f;
    const float reactor_ph = 0.0f;
    const float orp        = 0.0f;

    const app::scheduler::TimeHM now = app::scheduler::now();
    const app::scheduler::TimeSource src = app::scheduler::timeSource();

    char msg[256];
    snprintf(msg, sizeof(msg),
      "{"
        "\"water_temperature\":%.1f,"
        "\"heatsink_temperature\":%.1f,"
        "\"ambient_temperature\":%.1f,"
        "\"water_ph\":%.2f,"
        "\"reactor_ph\":%.2f,"
        "\"orp\":%.0f,"
        "\"clock_h\":%u,"
        "\"clock_m\":%u,"
        "\"clock_src\":\"%s\""
      "}",
      water_temperature,
      heatsink_temperature,
      ambient_temperature,
      water_ph,
      reactor_ph,
      orp,
      (unsigned)now.hour,
      (unsigned)now.minute,
      sourceToStr(src)
    );

    (void)hal::mqtt().publish(_stateTopic, (const uint8_t*)msg, strlen(msg), false);
  }

private:
  uint32_t _lastReconnectMs = 0;
  uint32_t _lastStateMs = 0;

  bool _discoveryPublished = false;
  bool _subscribed = false;

  char _stateTopic[96] = {0};
  char _availTopic[96] = {0};

  // cmd topics outlets 1..9 (índice 1..9)
  char _cmdOutletTopic[10][96] = {{0}};

  // C1.2 schedule cmd + cfg topics (índice 1..9)
  char _cmdScheduleTopic[10][96] = {{0}};
  char _cfgScheduleTopic[10][96] = {{0}};
  char _ackScheduleTopic[96] = {0};

  char _cmdCfgTopic[96] = {0};

  // rx small: schedule json is minimal; outlets are "0/1"
  char _rx[64] = {0};
};

ICommsBackend& comms_ha_singleton() {
  return CommsHABackend::instance();
}

} // namespace app
