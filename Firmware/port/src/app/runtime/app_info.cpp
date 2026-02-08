#include "app/runtime/app_info.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#include "app/config/app_config.h"
#include "app/runtime/app_boot_reason.h"

#include "hal/hal_mqtt.h"
#include "hal/hal_network.h"

#ifndef FERDUINO_FW_VERSION
  #define FERDUINO_FW_VERSION "dev"
#endif

#ifndef FERDUINO_BOARD
  #if defined(ARDUINO_AVR_MEGA2560)
    #define FERDUINO_BOARD "mega2560"
  #else
    #define FERDUINO_BOARD "unknown"
  #endif
#endif

namespace app::runtime {

static void ipToStr(char* out, size_t outLen, const hal::Ip4& ip) {
  snprintf(out, outLen, "%u.%u.%u.%u", ip.a, ip.b, ip.c, ip.d);
}

static void macToStr(char* out, size_t outLen, const uint8_t mac[6]) {
  snprintf(out, outLen, "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void publishInfoRetained() {
  const auto& cfg = app::cfg::get();
  const char* deviceId = cfg.mqtt.deviceId;
  if (!deviceId || deviceId[0] == '\0') return;

  char topic[96];
  snprintf(topic, sizeof(topic), "ferduino/%s/info", deviceId);

  char ip[20], gw[20], dns[20], sn[20], mac[24];
  ipToStr(ip, sizeof(ip), hal::network().localIp());
  ipToStr(gw, sizeof(gw), hal::network().gatewayIp());
  ipToStr(dns, sizeof(dns), hal::network().dnsServer());
  ipToStr(sn, sizeof(sn), hal::network().subnetMask());
  macToStr(mac, sizeof(mac), cfg.net.mac);

  const char* backend = (cfg.backendMode == app::cfg::BACKEND_HA) ? "HA" : "LEGACY";

  // linkStatus puede ser Unknown en W5100; aun así lo publicamos
  const int link = (int)hal::network().linkStatus();

  char json[360];
  snprintf(json, sizeof(json),
           "{"
             "\"fw\":\"%s\","
             "\"build\":\"%s %s\","
             "\"board\":\"%s\","
             "\"backend\":\"%s\","
             "\"boot\":\"%s\","
             "\"mcusr\":%u,"
             "\"mac\":\"%s\","
             "\"ip\":\"%s\","
             "\"gw\":\"%s\","
             "\"subnet\":\"%s\","
             "\"dns\":\"%s\","
             "\"link\":%d"
           "}",
           FERDUINO_FW_VERSION,
           __DATE__, __TIME__,
           FERDUINO_BOARD,
           backend,
           bootReason(),
           (unsigned)bootMcusr(),
           mac,
           ip, gw, sn, dns,
           link);

  (void)hal::mqtt().publish(topic, (const uint8_t*)json, strlen(json), true);
}

} // namespace app::runtime
