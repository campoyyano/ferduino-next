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
#include "app/alerts/forced_outlet_alert.h"
#include "app/scheduler/app_scheduler.h"
#include "app/scheduler/app_event_scheduler.h"

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

  if (!app::nvm::registry().begin()) {
    Serial.println("[runtime] WARN: NVM registry begin() failed");
  }
  if (!app::nvm::migrateLegacyIfNeeded()) {
    Serial.println("[runtime] WARN: legacy->registry migration skipped/failed");
  }

  if (!app::cfg::loadOrDefault()) {
    Serial.println("[runtime] WARN: cfg load failed; using defaults in RAM");
  }

  // B6.2a: Scheduler base (FAKE millis por defecto; RTC por flag+hook)
  app::scheduler::begin();

  // C1.1: Motor de eventos (ventanas ON/OFF) — solo calcula estado deseado
  app::scheduler::events::begin();

  // B6.1: Motor de outlets (RAM + registry TLV). En modo stub por defecto.
  app::outlets::begin();

  // C2.2: Alertas recurrentes si hay salidas en manual (auto=false)
  app::alerts::forced_outlets::begin();

  const auto& cfg = app::cfg::get();
  const hal::NetworkConfig net = toHalNetCfg(cfg.net);
  const hal::NetError ne = hal::network().begin(net);
  if (ne != hal::NetError::Ok) {
    Serial.print("[runtime] WARN: network begin() failed: ");
    Serial.println((int)ne);
  }

  app::comms().begin();

  g_prevMqttConnected = false;
}

void loop() {
  if (!g_started) begin();

  // B6.2a: tick scheduler
  app::scheduler::loop();

  // C1.1: tick motor de eventos (reacciona al cambio de minuto)
  app::scheduler::events::loop();

  // C2: aplicar estado deseado del scheduler SOLO a outlets en modo auto
  // (manual commands deshabilitan auto en app::outlets::set)
  for (uint8_t ch = 0; ch < app::outlets::count(); ++ch) {
    if (app::scheduler::events::consumeDesiredChanged(ch)) {
      (void)app::outlets::applyDesiredIfAuto(ch, app::scheduler::events::desiredOn(ch));
    }
  }

  hal::network().loop();
  app::comms().loop();
  app::alerts::forced_outlets::loop();

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
