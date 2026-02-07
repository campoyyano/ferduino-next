#include <Arduino.h>
#include <EEPROM.h>

#include "hal/hal_storage.h"

namespace hal {

// EEPROM total ATmega2560: 4096 bytes
static constexpr uint32_t EEPROM_TOTAL = 4096;

class StorageHalEeprom final : public IStorageHal {
public:
  bool begin() override {
    // AVR EEPROM no requiere inicialización.
    return true;
  }

  uint32_t capacity(StorageRegion region) const override {
    switch (region) {
      case StorageRegion::Config:  return EEPROM_TOTAL;
      case StorageRegion::Log:     return 0; // no soportado en B4.1
      case StorageRegion::Archive: return 0; // no soportado en B4.1
      default:                     return 0;
    }
  }

  bool read(StorageRegion region, uint32_t offset, void* dst, size_t len) override {
    if (region != StorageRegion::Config) return false;
    if (!dst && len) return false;
    if ((offset + (uint32_t)len) > EEPROM_TOTAL) return false;

    uint8_t* out = static_cast<uint8_t*>(dst);
    for (size_t i = 0; i < len; ++i) {
      out[i] = EEPROM.read((int)(offset + i));
    }
    return true;
  }

  bool write(StorageRegion region, uint32_t offset, const void* src, size_t len) override {
    if (region != StorageRegion::Config) return false;
    if (!src && len) return false;
    if ((offset + (uint32_t)len) > EEPROM_TOTAL) return false;

    const uint8_t* in = static_cast<const uint8_t*>(src);
    for (size_t i = 0; i < len; ++i) {
      // update() evita reescribir si el byte no cambia (mejor para vida de EEPROM).
      EEPROM.update((int)(offset + i), in[i]);
    }
    return true;
  }

  bool commit(StorageRegion region) override {
    // AVR EEPROM escribe inmediatamente; no hay commit real.
    return (region == StorageRegion::Config);
  }

  bool erase(StorageRegion region) override {
    if (region != StorageRegion::Config) return false;

    // Borrado lógico: 0xFF en toda la EEPROM.
    // OJO: esto escribe 4096 bytes -> usar solo en factory reset.
    for (uint32_t i = 0; i < EEPROM_TOTAL; ++i) {
      EEPROM.update((int)i, 0xFF);
    }
    return true;
  }
};

// Instancia única (sin heap)
static StorageHalEeprom g_storage;

IStorageHal& storage() {
  return g_storage;
}

} // namespace hal
