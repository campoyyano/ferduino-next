#include "app/runtime/app_runtime.h"

#include <Arduino.h>

#include "app/config/app_config.h"
#include "app/comms_backend.h"

#include "app/nvm/eeprom_registry.h"
#include "app/nvm/eeprom_migration.h"

#include "hal/hal_network.h"

namespace app::runtime {

static bool g_started = false;

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

static void printIp(const char* label, const hal::Ip4& ip) {
  Serial.print(label);
  Serial.print(": ");
  Serial.print(ip.a); Serial.print(".");
  Serial.print(ip.b); Serial.print(".");
  Serial.print(ip.c); Serial.print(".");
  Serial.println(ip.d);
}

void begin() {
  if (g_started) return;
  g_started = true;

  Serial.println("=== ferduino-next runtime (B5.4) ===");

  // 1) NVM registry + migración
  (void)app::nvm::registry().begin();
  (void)app::nvm::migrateLegacyIfNeeded();

  // 2) Carga config (registry TLV) o defaults (build_flags)
  (void)app::cfg::loadOrDefault();
  const auto& cfg = app::cfg::get();

  // 3) Arranque de red
  const hal::NetworkConfig net = toHalNetCfg(cfg.net);
  const hal::NetError ne = hal::network().begin(net);

  Serial.print("NET begin => ");
  Serial.println((int)ne);

  // 4) Backend comms (decide legacy/HA según cfg.backendMode)
  //    El backend inicializa MQTT y sus callbacks.
  app::comms().begin();

  // Info útil
  printIp("IP", hal::network().localIp());
  printIp("GW", hal::network().gatewayIp());
  printIp("SN", hal::network().subnetMask());
  printIp("DNS", hal::network().dnsServer());

  Serial.print("Backend mode: ");
  Serial.println((cfg.backendMode == app::cfg::BACKEND_HA) ? "HA" : "LEGACY");
}

void loop() {
  if (!g_started) begin();

  // Mantener stacks
  hal::network().loop();
  app::comms().loop();

  // Nota: de momento no añadimos delays “gordos” aquí.
  // Si necesitas throttle sin bloquear, añadimos scheduler/soft-timers.
}

} // namespace app::runtime
