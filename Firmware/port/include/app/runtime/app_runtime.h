#pragma once

namespace app::runtime {

// Arranque “mínimo real” (sin portar todavía módulos grandes):
// - NVM registry begin
// - migración legacy->registry si aplica
// - loadOrDefault config
// - network begin
// - comms begin (mqtt + callbacks)
void begin();

// Loop runtime mínimo: network.loop + comms.loop
void loop();

} // namespace app::runtime
