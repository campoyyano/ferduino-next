#pragma once

#include <stdint.h>
#include <stddef.h>

#include "app/nvm/eeprom_layout.h"
#include "hal/hal_storage.h"

namespace app::nvm {

// Tipos TLV soportados (mínimos para config).
enum class TlvType : uint8_t {
  U8    = 1,
  U16   = 2,
  U32   = 3,
  I32   = 4,
  Blob  = 5,
  Str   = 6, // bytes sin null-terminator
};

// Registro TLV simple sobre la zona REGISTRY_*
// Diseño: sin heap, con buffer interno (≈3KB) válido en Mega2560.
class EepromRegistry {
public:
  bool begin();               // lee header, valida CRC si existe
  bool isValid() const;       // header magic/version ok y crc ok (o vacío)
  bool format();              // inicializa header + payload vacío
  bool commit();              // escribe header+payload y crc

  uint16_t flags() const { return _hdr.flags; }
  bool setFlags(uint16_t flags); // actualiza flags y commit (o defer si editing)

  // Transacción opcional: permite agrupar múltiples set/remove en un solo commit.
  bool beginEdit();
  bool endEdit();
  bool isEditing() const { return _editing; }

  // Getters básicos
  bool getU32(uint16_t key, uint32_t& out) const;
  bool getI32(uint16_t key, int32_t& out) const;
  bool getBool(uint16_t key, bool& out) const;
  bool getStr(uint16_t key, char* out, size_t outLen) const; // null-terminator si cabe

  // Setters básicos
  bool setU32(uint16_t key, uint32_t v);
  bool setI32(uint16_t key, int32_t v);
  bool setBool(uint16_t key, bool v);
  bool setStr(uint16_t key, const char* s); // guarda sin '\0'

  bool remove(uint16_t key);

private:
  bool loadFromStorage();     // lee payload a RAM + valida
  bool rebuildHeaderCrc();    // recalcula crc32 del payload
  bool find(uint16_t key, size_t& entryOff, size_t& entrySize) const;

  // TLV entry: [key:u16][type:u8][len:u8][value:len]
  static constexpr uint16_t TLV_END_KEY = 0xFFFF;

  // Buffer payload (zona registry sin header)
  alignas(2) uint8_t _payload[REGISTRY_PAYLOAD_SIZE]{};
  RegistryHeader _hdr{};
  bool _valid = false;
  bool _editing = false;
};

// Service accessor (singleton sin heap)
EepromRegistry& registry();

} // namespace app::nvm
