#pragma once

#include <stddef.h>
#include <stdint.h>

namespace app::ha {

// Publica discovery mínimo (sensor + switch) para Home Assistant.
// Llamar después de conectar MQTT.
void publishDiscoveryMinimal();

// Permite re-publicar en el futuro si HA reinicia (opcional).
void publishDiscoveryAll();

} // namespace app::ha
