#pragma once

#include <stddef.h>
#include <stdint.h>

namespace app::cfgadmin {

// Topic de comando:
//   ferduino/<deviceId>/cmd/config
//
// Payloads soportados:
//   - "get"             -> publica JSON de config actual en ferduino/<deviceId>/cfg
//   - JSON (set parcial) -> aplica y guarda en EEPROM, responde en ferduino/<deviceId>/cfg/ack
//
// Keys soportadas en JSON (todas opcionales):
//   {"mqtt_host":"...", "mqtt_port":1883, "device_id":"...", "backend":"legacy|ha"}
//
// NOTA:
//   - Si cambias device_id o broker, se marca reboot_required=true (no reiniciamos solos).
//
// La función devuelve true si el mensaje era para cfgadmin (topic match) y fue procesado.
bool handleConfigCommand(const char* topic, const uint8_t* payload, size_t len);

} // namespace app::cfgadmin
