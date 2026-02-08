#pragma once

namespace app::nvm {

// Migra un subset de EEPROM legacy (0..1023) a registry TLV (>=1024).
// Es idempotente: si REGF_MIGRATED está set, no hace nada.
bool migrateLegacyIfNeeded();

} // namespace app::nvm
