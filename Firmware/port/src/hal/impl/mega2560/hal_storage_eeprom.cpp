#include "hal/hal_storage.h"

#include <EEPROM.h>

namespace hal {

// EEPROM interna del Mega2560: 4KB (0..4095)
// En nuestro layout, la región Config usa toda la EEPROM; la capa NVM
// decide qué offsets son legacy RO y cuáles registry RW.

class EepromStorageHal final : public IStorageHal {
public:
  bool begin() override { return true; }

  bool read(StorageRegion /*region*/, uint16_t offset, void* dst, size_t len) override {
    if (!dst && len) return false;
    uint8_t* p = static_cast<uint8_t*>(dst);
    for (size_t i = 0; i < len; ++i) {
      p[i] = EEPROM.read((int)(offset + (uint16_t)i));
    }
    return true;
  }

  bool write(StorageRegion /*region*/, uint16_t offset, const void* src, size_t len) override {
    if (!src && len) return false;
    const uint8_t* p = static_cast<const uint8_t*>(src);
    for (size_t i = 0; i < len; ++i) {
      EEPROM.update((int)(offset + (uint16_t)i), p[i]);
    }
    return true;
  }

  bool commit(StorageRegion /*region*/) override {
    // EEPROM interna: write/update es inmediato.
    return true;
  }
};

static EepromStorageHal g_storage;

IStorageHal& storage() { return g_storage; }

} // namespace hal
