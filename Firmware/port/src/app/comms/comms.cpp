#include "app/comms_backend.h"
#include "app/comms_mode.h"
#include "app/config/app_config.h"

namespace app {

// Factories (singletons) de backends
ICommsBackend& comms_legacy_singleton();
ICommsBackend& comms_ha_singleton();

ICommsBackend& comms() {
  // B3.2: selección runtime (EEPROM).
  // Importante: debes llamar a app::cfg::loadOrDefault() antes de comms().begin()
  // para que host/port/deviceId no queden vacíos.
  const auto& cfg = app::cfg::get();
  if (cfg.backendMode == app::cfg::BACKEND_HA) {
    return comms_ha_singleton();
  }
  return comms_legacy_singleton();
}

} // namespace app
