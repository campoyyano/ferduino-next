#include "app/nvm/eeprom_migration.h"

#include <stdint.h>
#include <stddef.h>

#include "app/nvm/eeprom_registry.h"
#include "app/nvm/eeprom_layout.h"
#include "hal/hal_storage.h"

namespace app::nvm {

// Keys (subset mínimo inicial)
static constexpr uint16_t KEY_TEMP_SET_10   = 100;
static constexpr uint16_t KEY_TEMP_OFF_10   = 101;
static constexpr uint16_t KEY_TEMP_ALARM_10 = 102;

// Offsets legacy (Ferduino original - Funcoes_EEPROM.h)
static constexpr uint16_t LEGACY_TEMP_SET_ADDR   = 482; // int16
static constexpr uint16_t LEGACY_TEMP_OFF_ADDR   = 484; // byte
static constexpr uint16_t LEGACY_TEMP_ALARM_ADDR = 485; // byte

static bool readLegacy(uint16_t addr, void* dst, size_t len) {
  return hal::storage().read(hal::StorageRegion::Config, addr, dst, len);
}

bool migrateLegacyIfNeeded() {
  auto& reg = registry();

  // Si el registry no es válido aún, lo inicializamos.
  if (!reg.isValid()) {
    if (!reg.format()) {
      return false;
    }
  }

  // Ya migrado
  if (reg.flags() & REGF_MIGRATED) {
    return true;
  }

  int16_t set10 = 0;
  uint8_t off10 = 0;
  uint8_t alarm10 = 0;

  // Si falla leer, nos quedamos con defaults (0). No abortamos.
  (void)readLegacy(LEGACY_TEMP_SET_ADDR, &set10, sizeof(set10));
  (void)readLegacy(LEGACY_TEMP_OFF_ADDR, &off10, sizeof(off10));
  (void)readLegacy(LEGACY_TEMP_ALARM_ADDR, &alarm10, sizeof(alarm10));

  // Legacy guarda en décimas (x10)
  (void)reg.setI32(KEY_TEMP_SET_10, (int32_t)set10);
  (void)reg.setI32(KEY_TEMP_OFF_10, (int32_t)off10);
  (void)reg.setI32(KEY_TEMP_ALARM_10, (int32_t)alarm10);

  // Marca migración completada
  return reg.setFlags((uint16_t)(reg.flags() | REGF_MIGRATED));
}

} // namespace app::nvm
