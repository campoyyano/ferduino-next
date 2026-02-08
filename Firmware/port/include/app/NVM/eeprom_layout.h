#pragma once

#include <stdint.h>

namespace app::nvm {

// EEPROM interna Mega2560: 4096 bytes
// Ferduino original usa (aprox) 0..1023 para settings legacy.
// En ferduino-next reservamos esa zona como LEGACY (read-only) y usamos
// la zona alta para el registry TLV.

static constexpr uint16_t EEPROM_TOTAL_SIZE = 4096;

// Zona legacy Ferduino original (NO escribir desde port; solo lectura / migración)
static constexpr uint16_t LEGACY_START = 0;
static constexpr uint16_t LEGACY_SIZE  = 1024;

// Zona registry TLV (RW)
static constexpr uint16_t REGISTRY_START = LEGACY_START + LEGACY_SIZE; // 1024
static constexpr uint16_t REGISTRY_SIZE  = EEPROM_TOTAL_SIZE - REGISTRY_START; // 3072

// Header (16 bytes) + payload
static constexpr uint16_t REGISTRY_HEADER_OFFSET  = REGISTRY_START;
static constexpr uint16_t REGISTRY_HEADER_SIZE    = 16;
static constexpr uint16_t REGISTRY_PAYLOAD_OFFSET = REGISTRY_HEADER_OFFSET + REGISTRY_HEADER_SIZE;
static constexpr uint16_t REGISTRY_PAYLOAD_SIZE   = (uint16_t)(REGISTRY_SIZE - REGISTRY_HEADER_SIZE);

// Magic/version del registry
static constexpr uint32_t REGISTRY_MAGIC   = 0x58475246UL; // 'F''R''G''X' little-endian
static constexpr uint16_t REGISTRY_VERSION = 1;

// Flags
static constexpr uint16_t REGF_NONE     = 0x0000;
static constexpr uint16_t REGF_MIGRATED = 0x0001; // Se ejecutó migración legacy->registry

// Header fijo del registry
struct RegistryHeader {
  uint32_t magic;
  uint16_t version;
  uint16_t flags;
  uint32_t crc32;   // CRC32 sobre payload completo; 0 => "vacío/no inicializado"
  uint32_t _rsvd;   // reservado (alineación/expansión)
};

} // namespace app::nvm
