#pragma once

#include <stdint.h>
#include <stddef.h>

// HAL de almacenamiento persistente.
// Diseñado para:
// - Hoy: EEPROM AVR (Config)
// - Mañana: FRAM pequeña (Config) + FRAM grande (Log) + SD (Archive)
//
// Nota: este header NO debe incluir headers de plataforma (EEPROM.h, Wire.h, etc.).
// La implementación vive en src/hal/impl/<platform>/.

namespace hal {

// Regiones lógicas de almacenamiento.
// En B4.1 solo implementamos Config (EEPROM AVR).
enum class StorageRegion : uint8_t {
  Config  = 0,  // parámetros / registry
  Log     = 1,  // logger (FRAM grande)
  Archive = 2,  // histórico (SD)
};

class IStorageHal {
public:
  virtual ~IStorageHal() = default;

  // Inicializa el backend (en AVR EEPROM suele ser no-op).
  virtual bool begin() = 0;

  // Capacidad de la región en bytes. 0 => región no soportada.
  virtual uint32_t capacity(StorageRegion region) const = 0;

  // Lectura/escritura en un offset dentro de la región.
  virtual bool read(StorageRegion region, uint32_t offset, void* dst, size_t len) = 0;
  virtual bool write(StorageRegion region, uint32_t offset, const void* src, size_t len) = 0;

  // Commit explícito:
  // - AVR EEPROM: no-op (true)
  // - Flash emulada / NVS: necesario
  // - SD: flush/sync
  virtual bool commit(StorageRegion region) = 0;

  // Borrado lógico de región (factory reset). Si no se soporta, false.
  virtual bool erase(StorageRegion region) = 0;
};

// Service locator (igual que hal::network()).
IStorageHal& storage();

} // namespace hal
