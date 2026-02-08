#pragma once

#include <stdint.h>
#include <stddef.h>

namespace hal {

// Región lógica de almacenamiento no volátil.
// En Mega2560 hoy solo usamos EEPROM interna, pero la interfaz permite
// migrar a FRAM/Flash/otros sin cambiar capas superiores.
enum class StorageRegion : uint8_t {
  Config = 0, // Configuración + registry TLV
  Log    = 1, // Futuro: logger
};

// HAL de almacenamiento no volátil.
// Contrato:
// - read/write operan sobre offsets absolutos dentro de la región.
// - commit() aplica los cambios si la implementación tiene buffer.
class IStorageHal {
public:
  virtual ~IStorageHal() = default;

  virtual bool begin() = 0;

  virtual bool read(StorageRegion region, uint16_t offset, void* dst, size_t len) = 0;
  virtual bool write(StorageRegion region, uint16_t offset, const void* src, size_t len) = 0;

  // En EEPROM interna suele ser no-op; en FRAM/Flash puede ser necesario.
  virtual bool commit(StorageRegion region) = 0;
};

// Acceso al singleton de almacenamiento.
IStorageHal& storage();

} // namespace hal
