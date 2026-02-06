#include "app/comms_backend.h"
#include "app/comms_mode.h"

namespace app {

// Factories (singletons) de backends
ICommsBackend& comms_legacy_singleton();
ICommsBackend& comms_ha_singleton();

ICommsBackend& comms() {
#if (FERDUINO_COMMS_MODE == FERDUINO_COMMS_HA)
  return comms_ha_singleton();
#else
  return comms_legacy_singleton();
#endif
}

} // namespace app
