#include "app/comms_backend.h"
#include "app/comms_mode.h"

namespace app {

// Esta función la implementa el backend legacy real
ICommsBackend& comms_legacy_singleton();

// Backend HA (stub por ahora)
class CommsHABackend final : public ICommsBackend {
public:
  void begin() override {}
  void loop() override {}
  bool connected() const override { return false; }

  bool publishStatus(const char*, const char*, bool=false) override {
    return false;
  }

  void onMqttMessage(const char*, const uint8_t*, size_t) override {}
};

static CommsHABackend g_ha;

// Selector de backend (compile-time)
ICommsBackend& comms() {
#if (FERDUINO_COMMS_MODE == FERDUINO_COMMS_HA)
  return g_ha;
#else
  return comms_legacy_singleton();
#endif
}

} // namespace app
