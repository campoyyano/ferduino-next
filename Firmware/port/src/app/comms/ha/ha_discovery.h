#pragma once

namespace app::ha {

// Publica discovery mínimo (en B2.2: publicamos todo lo que tenemos).
// Llamar después de conectar MQTT.
void publishDiscoveryMinimal();

// Publica discovery completo (alias de minimal por ahora).
void publishDiscoveryAll();

} // namespace app::ha
