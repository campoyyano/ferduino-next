#pragma once

#include <stdint.h>
#include <stddef.h>

// EEPROM layout para ferduino-next (Mega2560).
// Objetivo: compatibilidad con Ferduino original + zona nueva versionada.
//
// EEPROM total ATmega2560: 4096 bytes
// - Zona legacy (RO):     0..1023   (reservada para offsets del firmware original)
// - Zona registry (RW):   1024..4095 (header + payload TLV + CRC)
//
// NOTA:
// - En B4.2 solo fijamos el contrato (layout + header).
// - B4.3 implementa el registry TLV.
// - B4.4 implementa migración legacy -> registry.

namespace app::nvm {

static constexpr uint32_t EEPROM_TOTAL_BYTES = 4096;

// ========================
// Zona legacy (compat)
// ========================
// El firmware original usa offsets hasta ~871 (y LEDs en 1..~480).
// Reservamos 1KB completo para margen y estabilidad.
static constexpr uint32_t LEGACY_BASE      = 0;
static constexpr uint32_t LEGACY_SIZE      = 1024;
static constexpr uint32_t LEGACY_END_EXCL  = LEGACY_BASE + LEGACY_SIZE; // 1024

// ========================
// Zona nueva (registry)
// ========================
static constexpr uint32_t REGISTRY_BASE     = LEGACY_END_EXCL;     // 1024
static constexpr uint32_t REGISTRY_END_EXCL = EEPROM_TOTAL_BYTES;  // 4096
static constexpr uint32_t REGISTRY_SIZE     = REGISTRY_END_EXCL - REGISTRY_BASE; // 3072

// Magic 'FDNX' (ASCII) almacenado como uint32 little-endian.
// Bytes: 46 44 4E 58 ('F''D''N''X')
static constexpr uint32_t REGISTRY_MAGIC   = 0x584E4446u;
static constexpr uint16_t REGISTRY_VERSION = 1;

enum : uint16_t {
  REGF_NONE     = 0x0000,
  REGF_MIGRATED = 0x0001, // B4.4: migración legacy completada
};

// Header fijo al inicio de la zona registry.
// En B4.3 se calculará crc32 sobre el payload (zona posterior al header).
struct RegistryHeader {
  uint32_t magic;    // REGISTRY_MAGIC
  uint16_t version;  // REGISTRY_VERSION
  uint16_t flags;    // REGF_*
  uint32_t crc32;    // CRC del payload (0 si vacío/no inicializado)
};

static constexpr uint32_t REGISTRY_HEADER_OFFSET  = REGISTRY_BASE;
static constexpr uint32_t REGISTRY_PAYLOAD_OFFSET = REGISTRY_BASE + sizeof(RegistryHeader);
static constexpr uint32_t REGISTRY_PAYLOAD_SIZE   = REGISTRY_SIZE - sizeof(RegistryHeader);

static_assert(LEGACY_END_EXCL == REGISTRY_BASE, "EEPROM layout: boundary mismatch");
static_assert(REGISTRY_END_EXCL == EEPROM_TOTAL_BYTES, "EEPROM layout: total mismatch");
static_assert(REGISTRY_PAYLOAD_SIZE > 0, "EEPROM layout: payload must be > 0");

} // namespace app::nvm
