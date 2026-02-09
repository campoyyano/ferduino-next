#include "app/runtime/app_runtime.h"

#include <Arduino.h>

#include "app/config/app_config.h"
#include "app/comms_backend.h"

#include "app/nvm/eeprom_registry.h"
#include "app/nvm/eeprom_migration.h"

#include "app/runtime/app_status.h"
#include "app/runtime/app_info.h"
#include "app/runtime/app_telemetry.h"

#include "app/outlets/app_outlets.h"
#include "app/scheduler/app_scheduler.h"

#include "hal/hal_network.h"
#include "hal/hal_mqtt.h"

namespace app::runtime {

static bool g_started = false;
static bool g_prevMqttConnected = false;

static hal::Ip4 toHalIp(const app::cfg::Ip4& ip) {
  return hal::Ip4(ip.a, ip.b, ip.c, ip.d);
}

static hal::NetworkConfig toHalNetCfg(const app::cfg::NetworkConfig& n) {
  hal::NetworkConfig h;
  for (int i = 0; i < 6; ++i) h.mac[i] = n.mac[i];
  h.useDhcp = n.useDhcp;
  h.ip      = toHalIp(n.ip);
  h.gateway = toHalIp(n.gateway);
  h.subnet  = toHalIp(n.subnet);
  h.dns     = toHalIp(n.dns);
  return h;
}

void begin() {
  if (g_started) return;
  g_started = true;

  Serial.println("=== ferduino-next runtime (B5.4..B5.7) ===");

  (void)app::nvm::registry().begin();
  (void)app::nvm::migrateLegacyIfNeeded();

  (void)app::cfg::loadOrDefault();

  // B6.2a: Scheduler base (FAKE millis por defecto; RTC por flag+hook)
  app::scheduler::begin();

  // B6.1: Motor de outlets (RAM + registry TLV). En modo stub por defecto.
  app::outlets::begin();

  const auto& cfg = app::cfg::get();
  const hal::NetworkConfig net = toHalNetCfg(cfg.net);
  (void)hal::network().begin(net);

  app::comms().begin();

  g_prevMqttConnected = false;
}

void loop() {
  if (!g_started) begin();

  // B6.2a: tick scheduler
  app::scheduler::loop();

  hal::network().loop();
  app::comms().loop();

  const bool nowConn = hal::mqtt().connected();
  if (nowConn && !g_prevMqttConnected) {
    // Flanco de conexión: retained status + retained info
    app::runtime::publishStatusRetained();
    app::runtime::publishInfoRetained();
  }
  g_prevMqttConnected = nowConn;

  // Telemetría mínima (cada 30s)
  app::runtime::telemetryLoop(30);
}

} // namespace app::runtime
