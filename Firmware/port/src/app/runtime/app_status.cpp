#include "app/runtime/app_status.h"

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

namespace app::runtime {

static void ipToStr(char* out, size_t outLen, const hal::Ip4& ip) {
  snprintf(out, outLen, "%u.%u.%u.%u", ip.a, ip.b, ip.c, ip.d);
}

void publishStatusRetained() {
  const auto& cfg = app::cfg::get();

  const char* deviceId = cfg.mqtt.deviceId;
  if (!deviceId || deviceId[0] == '\0') {
    return;
  }

  char topic[96];
  snprintf(topic, sizeof(topic), "ferduino/%s/status", deviceId);

  char ip[20];
  ipToStr(ip, sizeof(ip), hal::network().localIp());

  const char* backend =
    (cfg.backendMode == app::cfg::BACKEND_HA) ? "HA" : "LEGACY";

  // JSON compacto, sin alloc dinámica
  char json[220];
  snprintf(json, sizeof(json),
           "{\"online\":1,\"backend\":\"%s\",\"boot\":\"%s\",\"mcusr\":%u,"
           "\"fw\":\"%s\",\"build\":\"%s %s\",\"ip\":\"%s\"}",
           backend,
           bootReason(),
           (unsigned)bootMcusr(),
           FERDUINO_FW_VERSION,
           __DATE__, __TIME__,
           ip);

  (void)hal::mqtt().publish(topic, (const uint8_t*)json, strlen(json), true);
}

} // namespace app::runtime
