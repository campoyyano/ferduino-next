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
  snprintf(out, outLen, "%s/%s/%s", FERDUINO_LEGACY_USERNAME, FERDUINO_LEGACY_APIKEY, suffix);
}

static void publishOk(const char* pubTopic) {
  const char ok[] = "{\"response\":\"ok\"}";
  (void)hal::mqtt().publish(pubTopic, (const uint8_t*)ok, sizeof(ok) - 1, false);
}

static void publishJson(const char* pubTopic, const char* json, bool retained = false) {
  if (!json) return;
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
    cfg.clientId = appcfg.mqtt.deviceId;
    cfg.keepAliveSec = 30;

    (void)hal::mqtt().begin(cfg);
    hal::mqtt().onMessage(&CommsLegacyBackend::onMqttMessageStatic);

    makeTopic(_subTopic, sizeof(_subTopic), "topic/command");
    makeTopic(_pubTopic, sizeof(_pubTopic), "topic/response");

    // admin config topic (siempre disponible aunque backend sea legacy)
    snprintf(_cmdCfgTopic, sizeof(_cmdCfgTopic),
             "ferduino/%s/cmd/config", appcfg.mqtt.deviceId);

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
        (void)hal::mqtt().subscribe(_subTopic);

        // subscribe admin config
        (void)hal::mqtt().subscribe(_cmdCfgTopic);

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

    char msg[160];
    snprintf(msg, sizeof(msg), "{\"%s\":\"%s\"}", key, value);

    return hal::mqtt().publish(
      _pubTopic,
      (const uint8_t*)msg,
      strlen(msg),
      retained
    ) == hal::MqttError::Ok;
  }

  void onMqttMessage(const char* topic,
                     const uint8_t* payload,
                     size_t len) override {
    if (!topic || !payload || len == 0)
      return;

    // B3.4: admin config primero
    if (app::cfgadmin::handleConfigCommand(topic, payload, len)) {
      return;
    }

    // Copia a buffer y termina en '\0'
    const size_t max = sizeof(_rxBuf) - 1;
    const size_t n = (len > max) ? max : len;
    memcpy(_rxBuf, payload, n);
    _rxBuf[n] = '\0';

    // Legacy terminator 'K'
    char* kpos = strchr(_rxBuf, 'K');
    if (!kpos)
      return;

    // Original: elimina la coma antes de K
    if (kpos > _rxBuf)
      *(kpos - 1) = '\0';

    // Tokenizar por comas
    int argc = 0;
    char* save = nullptr;
    char* p = _rxBuf;

    while (argc < (int)MAX_TOKENS) {
      char* tok = strtok_r(p, ",", &save);
      if (!tok) break;
      _argv[argc++] = tok;
      p = nullptr;
    }

    if (argc <= 0)
      return;

    const int id = atoi(_argv[0]);

    switch (id) {
      case 0: publishHome_ID0(); break;
      case 1: handleDs18b20_ID1(argc); break;
      case 2: handleDateTime_ID2(argc); break;
      case 3: handleCloudStorm_ID3(argc); break;
      case 4: handleLedTest_ID4(argc); break;
      case 5: handleLedTable_ID5(argc); break;
      case 6: handleMoonlight_ID6(argc); break;
      case 7: handleLedFan_ID7(argc); break;
      case 8: handleLedTempReduce_ID8(argc); break;
      case 9: handleWaterTemp_ID9(argc); break;
      case 10: handleManualDosing_ID10(argc); break;
      case 11: handleCalibration_ID11(argc); break;
      case 12: handleDoserSchedule_ID12(argc); break;
      case 13: handlePhControl_ID13(argc); break;
      case 14: handleReactorPh_ID14(argc); break;
      case 15: handleOrp_ID15(argc); break;
      case 16: handleDensity_ID16(argc); break;
      case 17: handleFilePump_ID17(argc); break;
      default:
        publishUnknown(id);
        break;
    }
  }

  static void onMqttMessageStatic(const char* topic,
                                 const uint8_t* payload,
                                 size_t len) {
    instance().onMqttMessage(topic, payload, len);
  }

  static CommsLegacyBackend& instance() {
    static CommsLegacyBackend inst;
    return inst;
  }

private:
  // ========== ID 0: Home ==========
  void publishHome_ID0() {
    const float tempH = 0.0f;
    const float tempC = 0.0f;
    const float tempA = 0.0f;
    const float PHA   = 0.0f;
    const float PHR   = 0.0f;
    const float ORP   = 0.0f;
    const float DEN   = 0.0f;

    const int wLedW   = 0;
    const int bLedW   = 0;
    const int rbLedW  = 0;
    const int redLedW = 0;
    const int uvLedW  = 0;

    const unsigned long running = millis() / 1000UL;
    const int speed = 0;

    const int moonPhase = 0;
    const int iluminated = 0;

    const char* dateStr = "";
    const char* timeStr = "";
    const int lastUpdate = 0;

    const int outlet1 = 0;
    const int outlet2 = 0;
    const int outlet3 = 0;
    const int outlet4 = 0;
    const int outlet5 = 0;
    const int outlet6 = 0;
    const int outlet7 = 0;
    const int outlet8 = 0;
    const int outlet9 = 0;

    const char* horaNuvem = "[0,0,0,0,0,0,0,0,0,0]";
    const int haveraNuvem = 0;
    const int haveraRelampago = 0;

    char msg[640];
    snprintf(
      msg, sizeof(msg),
      "{"
        "\"theatsink\":%.2f,"
        "\"twater\":%.2f,"
        "\"tamb\":%.2f,"
        "\"waterph\":%.2f,"
        "\"reactorph\":%.2f,"
        "\"orp\":%.2f,"
        "\"dens\":%.2f,"
        "\"wLedW\":%d,"
        "\"bLedW\":%d,"
        "\"rbLedW\":%d,"
        "\"redLedW\":%d,"
        "\"uvLedW\":%d,"
        "\"running\":%lu,"
        "\"speed\":%d,"
        "\"moonPhase\":%d,"
        "\"iluminated\":%d,"
        "\"date\":\"%s\","
        "\"time\":\"%s\","
        "\"update\":%d,"
        "\"outlet1\":%d,"
        "\"outlet2\":%d,"
        "\"outlet3\":%d,"
        "\"outlet4\":%d,"
        "\"outlet5\":%d,"
        "\"outlet6\":%d,"
        "\"outlet7\":%d,"
        "\"outlet8\":%d,"
        "\"outlet9\":%d,"
        "\"horaNuvem\":\"%s\","
        "\"haveraNuvem\":%d,"
        "\"haveraRelampago\":%d"
      "}",
      D(tempH), D(tempC), D(tempA),
      D(PHA), D(PHR), D(ORP), D(DEN),
      wLedW, bLedW, rbLedW, redLedW, uvLedW,
      running, speed,
      moonPhase, iluminated,
      dateStr, timeStr, lastUpdate,
      outlet1, outlet2, outlet3, outlet4, outlet5, outlet6, outlet7, outlet8, outlet9,
      horaNuvem, haveraNuvem, haveraRelampago
    );

    publishJson(_pubTopic, msg, false);

    // Bridge Legacy -> HA (B2.2)
    app::ha::bridgeFromLegacyHomeJson(msg);
  }

  // ========== ID 1 ==========
  void handleDs18b20_ID1(int argc) {
    const int mode = (argc >= 2) ? atoi(_argv[1]) : 0;

    if (mode == 0) {
      const int number = 0;
      const float p1 = 0.0f, p2 = 0.0f, p3 = 0.0f;
      const int ap1 = 0, ap2 = 0, ap3 = 0;

      char msg[220];
      snprintf(msg, sizeof(msg),
               "{"
                 "\"number\":%d,"
                 "\"p1\":%.2f,\"p2\":%.2f,\"p3\":%.2f,"
                 "\"ap1\":%d,\"ap2\":%d,\"ap3\":%d"
               "}",
               number, D(p1), D(p2), D(p3), ap1, ap2, ap3);

      publishJson(_pubTopic, msg, false);
    } else {
      publishOk(_pubTopic);
    }
  }

  // ========== ID 2 ==========
  void handleDateTime_ID2(int argc) {
    const int mode = (argc >= 2) ? atoi(_argv[1]) : 0;

    if (mode == 0) {
      const char* dateStr = "";
      const char* timeStr = "";

      char msg[96];
      snprintf(msg, sizeof(msg), "{\"date\":\"%s\",\"time\":\"%s\"}", dateStr, timeStr);
      publishJson(_pubTopic, msg, false);
    } else {
      publishOk(_pubTopic);
    }
  }

  // ========== ID 3 ==========
  void handleCloudStorm_ID3(int argc) {
    const int mode = (argc >= 2) ? atoi(_argv[1]) : 0;

    if (mode == 0) {
      const int status = 0;
      const int hour = 0;
      const int minute = 0;
      const int duration = 0;

      const int monday=0,tuesday=0,wednesday=0,thursday=0,friday=0,saturday=0,sunday=0;

      char msg[260];
      snprintf(msg, sizeof(msg),
               "{"
                 "\"status\":%d,"
                 "\"hour\":%d,\"minute\":%d,\"duration\":%d,"
                 "\"monday\":%d,\"tuesday\":%d,\"wednesday\":%d,\"thursday\":%d,"
                 "\"friday\":%d,\"saturday\":%d,\"sunday\":%d"
               "}",
               status, hour, minute, duration,
               monday,tuesday,wednesday,thursday,friday,saturday,sunday);

      publishJson(_pubTopic, msg, false);
    } else {
      publishOk(_pubTopic);
    }
  }

  // ========== ID 4 ==========
  void handleLedTest_ID4(int argc) {
    const int mode = (argc >= 2) ? atoi(_argv[1]) : 0;

    if (mode == 0) {
      const int wLedW=0,bLedW=0,rbLedW=0,redLedW=0,uvLedW=0;

      char msg[160];
      snprintf(msg, sizeof(msg),
               "{"
                 "\"wLedW\":%d,\"bLedW\":%d,\"rbLedW\":%d,\"redLedW\":%d,\"uvLedW\":%d"
               "}",
               wLedW,bLedW,rbLedW,redLedW,uvLedW);

      publishJson(_pubTopic, msg, false);
    } else {
      publishOk(_pubTopic);
    }
  }

  // ========== ID 5 ==========
  void handleLedTable_ID5(int argc) {
    const int mode = (argc >= 2) ? atoi(_argv[1]) : 0;

    if (mode == 0) {
      const int v0=0,v1=0,v2=0,v3=0,v4=0,v5=0,v6=0,v7=0;

      char msg[220];
      snprintf(msg, sizeof(msg),
               "{"
                 "\"value0\":%d,\"value1\":%d,\"value2\":%d,\"value3\":%d,"
                 "\"value4\":%d,\"value5\":%d,\"value6\":%d,\"value7\":%d"
               "}",
               v0,v1,v2,v3,v4,v5,v6,v7);

      publishJson(_pubTopic, msg, false);
    } else {
      publishOk(_pubTopic);
    }
  }

  // ========== ID 6 ==========
  void handleMoonlight_ID6(int argc) {
    const int mode = (argc >= 2) ? atoi(_argv[1]) : 0;

    if (mode == 0) {
      const int minv=0, maxv=0;
      char msg[96];
      snprintf(msg, sizeof(msg), "{\"min\":%d,\"max\":%d}", minv, maxv);
      publishJson(_pubTopic, msg, false);
    } else {
      publishOk(_pubTopic);
    }
  }

  // ========== ID 7 ==========
  void handleLedFan_ID7(int argc) {
    const int mode = (argc >= 2) ? atoi(_argv[1]) : 0;

    if (mode == 0) {
      const int minfan=0, maxfan=0;
      char msg[120];
      snprintf(msg, sizeof(msg), "{\"minfan\":%d,\"maxfan\":%d}", minfan, maxfan);
      publishJson(_pubTopic, msg, false);
    } else {
      publishOk(_pubTopic);
    }
  }

  // ========== ID 8 ==========
  void handleLedTempReduce_ID8(int argc) {
    const int mode = (argc >= 2) ? atoi(_argv[1]) : 0;

    if (mode == 0) {
      const int templimit=0, potred=0;
      char msg[140];
      snprintf(msg, sizeof(msg), "{\"templimit\":%d,\"potred\":%d}", templimit, potred);
      publishJson(_pubTopic, msg, false);
    } else {
      publishOk(_pubTopic);
    }
  }

  // ========== ID 9 ==========
  void handleWaterTemp_ID9(int argc) {
    const int mode = (argc >= 2) ? atoi(_argv[1]) : 0;

    if (mode == 0) {
      const float setTemp=0.0f, offTemp=0.0f, alarmTemp=0.0f;
      char msg[180];
      snprintf(msg, sizeof(msg),
               "{\"setTemp\":%.2f,\"offTemp\":%.2f,\"alarmTemp\":%.2f}",
               D(setTemp), D(offTemp), D(alarmTemp));
      publishJson(_pubTopic, msg, false);
    } else {
      publishOk(_pubTopic);
    }
  }

  // ========== ID 10 ==========
  void handleManualDosing_ID10(int /*argc*/) {
    const int waitSec = 0;
    char msg[96];
    snprintf(msg, sizeof(msg), "{\"wait\":\"%d\"}", waitSec);
    publishJson(_pubTopic, msg, false);
  }

  // ========== ID 11 ==========
  void handleCalibration_ID11(int argc) {
    const int mode = (argc >= 2) ? atoi(_argv[1]) : 0;

    if (mode == 0) {
      const float calib=0.0f;
      const int waitSec=0;
      char msg[140];
      snprintf(msg, sizeof(msg), "{\"calib\":%.2f,\"wait\":\"%d\"}", D(calib), waitSec);
      publishJson(_pubTopic, msg, false);
    } else {
      publishOk(_pubTopic);
    }
  }

  // ========== ID 12 ==========
  void handleDoserSchedule_ID12(int argc) {
    const int mode = (argc >= 2) ? atoi(_argv[1]) : 0;

    if (mode == 0) {
      const int onoff=0;
      const int quantity=0;
      const float dose=0.0f;
      const int hStart=0,mStart=0,hEnd=0,mEnd=0;
      const int monday=0,tuesday=0,wednesday=0,thursday=0,friday=0,saturday=0,sunday=0;

      char msg[360];
      snprintf(msg, sizeof(msg),
               "{"
                 "\"onoff\":%d,"
                 "\"quantity\":%d,"
                 "\"dose\":%.2f,"
                 "\"hStart\":%d,\"mStart\":%d,"
                 "\"hEnd\":%d,\"mEnd\":%d,"
                 "\"monday\":%d,\"tuesday\":%d,\"wednesday\":%d,\"thursday\":%d,"
                 "\"friday\":%d,\"saturday\":%d,\"sunday\":%d"
               "}",
               onoff, quantity, D(dose),
               hStart, mStart, hEnd, mEnd,
               monday,tuesday,wednesday,thursday,friday,saturday,sunday);

      publishJson(_pubTopic, msg, false);
    } else {
      publishOk(_pubTopic);
    }
  }

  // ========== ID 13 ==========
  void handlePhControl_ID13(int argc) {
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

  // ========== ID 14 ==========
  void handleReactorPh_ID14(int argc) {
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

  char _subTopic[64] = {0};
  char _pubTopic[64] = {0};
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
