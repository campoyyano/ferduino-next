#pragma once

namespace app::runtime {

// Arranque mínimo real:
// - cfg load
// - network begin
// - comms begin
void begin();

// Loop runtime mínimo (mantener stacks + status)
void loop();

} // namespace app::runtime
