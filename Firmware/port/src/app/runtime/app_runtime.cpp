#include "app/runtime/app_runtime.h"

#include <Arduino.h>

#include "app/config/app_config.h"
#include "app/comms_backend.h"

#include "app/nvm/eeprom_registry.h"
#include "app/nvm/eeprom_migration.h"

#include "app/runtime/app_status.h"
#include "app/runtime/app_info.h"
#include "app/runtime/app_telemetry.h"
#include "app/runtime/app_tempctrl_telemetry.h"

#include "app/outlets/app_outlets.h"
#include "app/scheduler/app_scheduler.h"
#include "app/scheduler/app_event_scheduler.h"

#include "app/sensors/sensors.h"
#include "app/temp_control/temp_control.h"

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

  Serial.println("=== ferduino-next runtime ===");

  if (!app::nvm::registry().begin()) {
    Serial.println("[runtime] WARN: NVM registry begin() failed");
  }
  if (!app::nvm::migrateLegacyIfNeeded()) {
    Serial.println("[runtime] WARN: legacy->registry migration skipped/failed");
  }

  if (!app::cfg::loadOrDefault()) {
    Serial.println("[runtime] WARN: cfg load failed; using defaults in RAM");
  }

  // Scheduler base (FAKE millis por defecto; RTC por flag+hook)
  app::scheduler::begin();

  // Motor de eventos (ventanas ON/OFF)
  app::scheduler::events::begin();

  // Outlets (RAM + registry TLV). En modo stub por defecto.
  app::outlets::begin();

  // Sensores (FAKE por defecto)
  app::sensors::begin();

  // Control de temperatura (lógica; GPIO real opcional por flag)
  app::tempctrl::begin();

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

  // Tick scheduler
  app::scheduler::loop();

  // Tick motor de eventos
  app::scheduler::events::loop();

  // Tick sensores
  app::sensors::loop();

  // Tick control temperatura
  app::tempctrl::loop();

  hal::network().loop();
  app::comms().loop();

  const bool nowConn = hal::mqtt().connected();
  if (nowConn && !g_prevMqttConnected) {
    // Flanco de conexión: retained status + retained info
    app::runtime::publishStatusRetained();
    app::runtime::publishInfoRetained();
  }
  g_prevMqttConnected = nowConn;

  // Telemetría mínima existente (cada 30s)
  app::runtime::telemetryLoop(30);

  // NUEVO: telemetría tempctrl (cada 10s)
  app::runtime::tempctrlTelemetryLoop(10);
}

} // namespace app::runtime
