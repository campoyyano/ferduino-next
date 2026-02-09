#include "app/comms_backend.h"
#include "app/comms_mode.h"
#include "app/comms/ha/ha_legacy_bridge.h"

#include "app/config/app_config.h"
#include "app/config/app_config_mqtt_admin.h"

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
  if (!json) return;
  if (!pubTopic || pubTopic[0] == '\0') return;
  (void)hal::mqtt().publish(pubTopic, (const uint8_t*)json, strlen(json), retained);
}

/* ==== Backend Legacy ==== */

class CommsLegacyBackend final : public ICommsBackend {
public:
  void begin() override {
    // No hay setup() global todavía -> garantizamos cfg en RAM
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

    // Public topic: response en "username/apikey/topic/response"
    // TODO: completar IDs gradualmente (B6+)
    switch (id) {
      case 0:  handleTemps_ID0(argc); break;
      case 1:  handleLeds_ID1(argc); break;
      case 2:  handleRelay_ID2(argc); break;
      case 3:  handleAlimentador_ID3(argc); break;
      case 4:  handleTPA_ID4(argc); break;
      case 5:  handleTimers_ID5(argc); break;
      case 6:  handlePwmLeds_ID6(argc); break;
      case 7:  handlePwmLeds2_ID7(argc); break;
      case 8:  handlePwmLeds3_ID8(argc); break;
      case 9:  handlePwmLeds4_ID9(argc); break;
      case 10: handlePwmLeds5_ID10(argc); break;
      case 11: handleDosadora_ID11(argc); break;
      case 12: handleDosadoraDay_ID12(argc); break;
      case 13: handlePhReactor_ID13(argc); break;
      case 14: handlePhAquarium_ID14(argc); break;
      case 15: handleOrp_ID15(argc); break;
      case 16: handleDensity_ID16(argc); break;
      case 17: handleFilePump_ID17(argc); break;
      default: publishUnknown(id); break;
    }
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
               D(setTemp), D(offTemp), D(alarmTemp));
      publishJson(_pubTopic, msg, false);
    } else publishOk(_pubTopic);
  }

  // ========== ID 1 ==========
  void handleLeds_ID1(int argc) {
    const int mode = (argc >= 2) ? atoi(_argv[1]) : 0;
    if (mode == 0) {
      const float setW=0.0f, setB=0.0f, setRB=0.0f, setR=0.0f, setUV=0.0f;
      char msg[200];
      snprintf(msg, sizeof(msg),
               "{\"setW\":%.2f,\"setB\":%.2f,\"setRB\":%.2f,\"setR\":%.2f,\"setUV\":%.2f}",
               D(setW), D(setB), D(setRB), D(setR), D(setUV));
      publishJson(_pubTopic, msg, false);
    } else publishOk(_pubTopic);
  }

  // ========== ID 2 ==========
  void handleRelay_ID2(int argc) {
    (void)argc;
    publishOk(_pubTopic);
  }

  // ========== ID 3 ==========
  void handleAlimentador_ID3(int argc) {
    (void)argc;
    publishOk(_pubTopic);
  }

  // ========== ID 4 ==========
  void handleTPA_ID4(int argc) {
    (void)argc;
    publishOk(_pubTopic);
  }

  // ========== ID 5 ==========
  void handleTimers_ID5(int argc) {
    (void)argc;
    publishOk(_pubTopic);
  }

  // ========== ID 6 ==========
  void handlePwmLeds_ID6(int argc) {
    (void)argc;
    publishOk(_pubTopic);
  }

  // ========== ID 7 ==========
  void handlePwmLeds2_ID7(int argc) {
    (void)argc;
    publishOk(_pubTopic);
  }

  // ========== ID 8 ==========
  void handlePwmLeds3_ID8(int argc) {
    (void)argc;
    publishOk(_pubTopic);
  }

  // ========== ID 9 ==========
  void handlePwmLeds4_ID9(int argc) {
    (void)argc;
    publishOk(_pubTopic);
  }

  // ========== ID 10 ==========
  void handlePwmLeds5_ID10(int argc) {
    (void)argc;
    publishOk(_pubTopic);
  }

  // ========== ID 11 ==========
  void handleDosadora_ID11(int argc) {
    (void)argc;
    publishOk(_pubTopic);
  }

  // ========== ID 12 ==========
  void handleDosadoraDay_ID12(int argc) {
    (void)argc;
    publishOk(_pubTopic);
  }

  // ========== ID 13 ==========
  void handlePhReactor_ID13(int argc) {
    const int mode = (argc >= 2) ? atoi(_argv[1]) : 0;
    if (mode == 0) {
      const float setPHR=0.0f, offPHR=0.0f, alarmPHR=0.0f;
      char msg[200];
      snprintf(msg, sizeof(msg),
               "{\"setPHR\":%.2f,\"offPHR\":%.2f,\"alarmPHR\":%.2f}",
               D(setPHR), D(offPHR), D(alarmPHR));
      publishJson(_pubTopic, msg, false);
    } else publishOk(_pubTopic);
  }

  // ========== ID 14 ==========
  void handlePhAquarium_ID14(int argc) {
    const int mode = (argc >= 2) ? atoi(_argv[1]) : 0;
    if (mode == 0) {
      const float setPHA=0.0f, offPHA=0.0f, alarmPHA=0.0f;
      char msg[200];
      snprintf(msg, sizeof(msg),
               "{\"setPHA\":%.2f,\"offPHA\":%.2f,\"alarmPHA\":%.2f}",
               D(setPHA), D(offPHA), D(alarmPHA));
      publishJson(_pubTopic, msg, false);
    } else publishOk(_pubTopic);
  }

  // ========== ID 15 ==========
  void handleOrp_ID15(int argc) {
    const int mode = (argc >= 2) ? atoi(_argv[1]) : 0;
    if (mode == 0) {
      const float setORP=0.0f, offORP=0.0f, alarmORP=0.0f;
      char msg[200];
      snprintf(msg, sizeof(msg),
               "{\"setORP\":%.2f,\"offORP\":%.2f,\"alarmORP\":%.2f}",
               D(setORP), D(offORP), D(alarmORP));
      publishJson(_pubTopic, msg, false);
    } else publishOk(_pubTopic);
  }

  // ========== ID 16 ==========
  void handleDensity_ID16(int argc) {
    const int mode = (argc >= 2) ? atoi(_argv[1]) : 0;
    if (mode == 0) {
      const float setDEN=0.0f, offDEN=0.0f, alarmDEN=0.0f;
      char msg[200];
      snprintf(msg, sizeof(msg),
               "{\"setDEN\":%.2f,\"offDEN\":%.2f,\"alarmDEN\":%.2f}",
               D(setDEN), D(offDEN), D(alarmDEN));
      publishJson(_pubTopic, msg, false);
    } else publishOk(_pubTopic);
  }

  // ========== ID 17 ==========
  void handleFilePump_ID17(int argc) {
    int idx = 0;
    if (argc >= 3) idx = atoi(_argv[2]); // original usa inParse[2]
    if (idx < 0) idx = 0;

    char msg[96];
    snprintf(msg, sizeof(msg), "{\"filepump%d\":\"0000\"}", idx);
    publishJson(_pubTopic, msg, false);
  }

  void publishUnknown(int id) {
    char msg[96];
    snprintf(msg, sizeof(msg), "{\"error\":\"unknown_id\",\"id\":%d}", id);
    publishJson(_pubTopic, msg, false);
  }

private:
  static constexpr size_t RX_BUF_SIZE = 256;
  static constexpr size_t MAX_TOKENS  = 32;

  char _subTopic[128] = {0};
  char _pubTopic[128] = {0};
  char _cmdCfgTopic[96] = {0};

  uint32_t _lastReconnectMs = 0;

  char _rxBuf[RX_BUF_SIZE] = {0};
  char* _argv[MAX_TOKENS]  = {0};
};

/* ==== Factory hook ==== */

ICommsBackend& comms_legacy_singleton() {
  return CommsLegacyBackend::instance();
}

} // namespace app
