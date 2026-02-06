#include <Arduino.h>

#include "hal/hal_network.h"

// Si tienes un HAL de serial/log, puedes cambiarlo luego.
// De momento Serial directo para validar red.

static void printIp(const char* label, const hal::Ip4& ip) {
  Serial.print(label);
  Serial.print(": ");
  Serial.print(ip.a); Serial.print('.');
  Serial.print(ip.b); Serial.print('.');
  Serial.print(ip.c); Serial.print('.');
  Serial.println(ip.d);
}

void app_network_smoketest() {
  Serial.begin(115200);
  while (!Serial) { /* Mega normalmente no bloquea, pero ok */ }

  Serial.println();
  Serial.println("=== Network smoketest (A9.2) ===");

  hal::NetworkConfig cfg;

  // MAC de prueba (ADMINISTRATIVELY LOCAL: 02:xx:xx:xx:xx:xx)
  cfg.mac[0] = 0x02;
  cfg.mac[1] = 0xF0;
  cfg.mac[2] = 0xED;
  cfg.mac[3] = 0x00;
  cfg.mac[4] = 0x00;
  cfg.mac[5] = 0x01;

  cfg.useDhcp = true;

  const auto err = hal::network().begin(cfg);
  if (err != hal::NetError::Ok) {
    Serial.print("Network begin() failed, err=");
    Serial.println((int)err);
    for (;;) delay(1000);
  }

  Serial.println("Network initialized.");

  printIp("IP",      hal::network().localIp());
  printIp("GW",      hal::network().gatewayIp());
  printIp("MASK",    hal::network().subnetMask());
  printIp("DNS",     hal::network().dnsServer());

  for (;;) {
    // En W5x00 suele ser no-op, pero mantenemos la API
    hal::network().loop();

    const auto ls = hal::network().linkStatus();
    Serial.print("Link: ");
    if (ls == hal::LinkStatus::Up) Serial.println("UP");
    else if (ls == hal::LinkStatus::Down) Serial.println("DOWN");
    else Serial.println("UNKNOWN");

    delay(2000);
  }
}
