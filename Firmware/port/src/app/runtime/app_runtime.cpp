#include "app/runtime/app_runtime.h"

#include <Arduino.h>

#include "app/config/app_config.h"
#include "app/comms_backend.h"

#include "app/nvm/eeprom_registry.h"
#include "app/nvm/eeprom_migration.h"

#include "app/runtime/app_status.h"

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

  Serial.println("=== ferduino-next runtime (B5.4/B5.5) ===");

  // 1) NVM registry + migración
  (void)app::nvm::registry().begin();
  (void)app::nvm::migrateLegacyIfNeeded();

  // 2) Config
  (void)app::cfg::loadOrDefault();

  // 3) Network
  const auto& cfg = app::cfg::get();
  const hal::NetworkConfig net = toHalNetCfg(cfg.net);
  (void)hal::network().begin(net);

  // 4) Comms backend (HA/LEGACY) + MQTT init/callbacks
  app::comms().begin();

  g_prevMqttConnected = false;
}

void loop() {
  if (!g_started) begin();

  // Mantener stacks (los backends también llaman a network/mqtt loop,
  // pero aquí no rompe: network.loop() es no-op portable).
  hal::network().loop();

  // El backend es el que suele llevar la reconexión MQTT.
  app::comms().loop();

  const bool nowConn = hal::mqtt().connected();
  if (nowConn && !g_prevMqttConnected) {
    // Flanco de conexión: publicar status retained
    app::runtime::publishStatusRetained();
  }
  g_prevMqttConnected = nowConn;
}

} // namespace app::runtime
