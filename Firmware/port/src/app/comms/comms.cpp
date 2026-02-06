#include "app/comms_backend.h"
#include "app/comms_mode.h"

namespace app {

// Stubs (A10.1): compilan pero no hacen nada.
// En A10.2/A10.3 se implementan de verdad.

class CommsLegacyBackend final : public ICommsBackend {
public:
  void begin() override {}
  void loop() override {}
  bool connected() const override { return false; }

  bool publishStatus(const char* key, const char* value, bool retained=false) override {
    (void)key; (void)value; (void)retained;
    return false;
  }

  void onMqttMessage(const char* topic, const uint8_t* payload, size_t len) override {
    (void)topic; (void)payload; (void)len;
  }
};

class CommsHABackend final : public ICommsBackend {
public:
  void begin() override {}
  void loop() override {}
  bool connected() const override { return false; }

  bool publishStatus(const char* key, const char* value, bool retained=false) override {
    (void)key; (void)value; (void)retained;
    return false;
  }

  void onMqttMessage(const char* topic, const uint8_t* payload, size_t len) override {
    (void)topic; (void)payload; (void)len;
  }
};

static CommsLegacyBackend g_legacy;
static CommsHABackend     g_ha;

ICommsBackend& comms() {
#if (FERDUINO_COMMS_MODE == FERDUINO_COMMS_HA)
  return g_ha;
#else
  return g_legacy;
#endif
}

} // namespace app
