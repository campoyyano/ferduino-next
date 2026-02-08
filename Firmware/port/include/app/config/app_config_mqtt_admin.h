#pragma once

#include <stddef.h>
#include <stdint.h>

namespace app::cfgadmin {

// Topic de comando:
//   ferduino/<deviceId>/cmd/config
//
// Payloads soportados:
//   - "get" (o "GET" o "?")  -> publica JSON de config actual en ferduino/<deviceId>/cfg (retained)
//   - JSON (set parcial)     -> aplica cambios, guarda en registry TLV y responde en ferduino/<deviceId>/cfg/ack
//
// Keys soportadas en JSON (todas opcionales):
//   {
//     "backend":  "legacy" | "ha",
//     "mqtt_host":"...", "mqtt_port":1883, "device_id":"...",
//     "net_dhcp":true,
//     "net_ip":"192.168.1.50", "net_gw":"192.168.1.1", "net_subnet":"255.255.255.0", "net_dns":"8.8.8.8",
//     "net_mac":"02:FD:00:00:00:01"
//   }
//
// NOTA:
// - Si cambias broker/deviceId/backend o parámetros de red, se marca reboot_required=true
//   (no reiniciamos automáticamente).
//
// La función devuelve true si el mensaje era para cfgadmin (topic match) y fue procesado.
bool handleConfigCommand(const char* topic, const uint8_t* payload, size_t len);

} // namespace app::cfgadmin
