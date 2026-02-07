#include "app/nvm/eeprom_registry.h"

#include <string.h>

namespace app::nvm {

static EepromRegistry g_reg;

// CRC32 (tableless) - tamaño pequeño, suficiente para AVR
static uint32_t crc32_update(uint32_t crc, const uint8_t* data, size_t len) {
  crc = ~crc;
  for (size_t i = 0; i < len; ++i) {
    crc ^= data[i];
    for (uint8_t b = 0; b < 8; ++b) {
      const uint32_t mask = (uint32_t)-(int32_t)(crc & 1u);
      crc = (crc >> 1) ^ (0xEDB88320u & mask);
    }
  }
  return ~crc;
}

static uint32_t crc32(const uint8_t* data, size_t len) {
  return crc32_update(0u, data, len);
}

EepromRegistry& registry() { return g_reg; }

bool EepromRegistry::begin() {
  return loadFromStorage();
}

bool EepromRegistry::isValid() const {
  return _valid;
}

bool EepromRegistry::format() {
  // payload vacío con TLV terminator
  memset(_payload, 0xFF, sizeof(_payload));
  _payloadLen = 0;

  _hdr.magic = REGISTRY_MAGIC;
  _hdr.version = REGISTRY_VERSION;
  _hdr.flags = REGF_NONE;

  // En payload vacío, CRC marcado como 0 para indicar "vacío/no inicializado".
  _hdr.crc32 = 0;

  _valid = true;
  return commit();
}

bool EepromRegistry::commit() {
  if (_hdr.magic != REGISTRY_MAGIC || _hdr.version != REGISTRY_VERSION) {
    return false;
  }

  // Recalcula CRC (si hay payload real)
  rebuildHeaderCrc();

  auto& st = hal::storage();
  if (!st.begin()) return false;

  // Escribe header
  if (!st.write(hal::StorageRegion::Config, REGISTRY_HEADER_OFFSET, &_hdr, sizeof(_hdr))) return false;

  // Escribe payload (zona completa para simplicidad/consistencia)
  if (!st.write(hal::StorageRegion::Config, REGISTRY_PAYLOAD_OFFSET, _payload, REGISTRY_PAYLOAD_SIZE)) return false;

  return st.commit(hal::StorageRegion::Config);
}

bool EepromRegistry::setFlags(uint16_t flags) {
  if (!_valid) return false;
  _hdr.flags = flags;
  return commit();
}

bool EepromRegistry::loadFromStorage() {
  auto& st = hal::storage();
  if (!st.begin()) return false;

  RegistryHeader h{};
  if (!st.read(hal::StorageRegion::Config, REGISTRY_HEADER_OFFSET, &h, sizeof(h))) return false;

  _hdr = h;

  // Header vacío / sin inicializar
  if (_hdr.magic != REGISTRY_MAGIC) {
    _valid = false;
    return true; // no es error, solo "no inicializado"
  }
  if (_hdr.version != REGISTRY_VERSION) {
    _valid = false;
    return true;
  }

  if (!st.read(hal::StorageRegion::Config, REGISTRY_PAYLOAD_OFFSET, _payload, REGISTRY_PAYLOAD_SIZE)) {
    _valid = false;
    return false;
  }

  // Valida CRC si está definido (crc32==0 => consideramos "vacío/skip")
  if (_hdr.crc32 != 0) {
    const uint32_t c = crc32(_payload, REGISTRY_PAYLOAD_SIZE);
    _valid = (c == _hdr.crc32);
  } else {
    _valid = true;
  }

  return true;
}

bool EepromRegistry::rebuildHeaderCrc() {
  // Si todo el payload está 0xFF, tratamos como vacío (crc=0)
  bool anyNotFF = false;
  for (size_t i = 0; i < REGISTRY_PAYLOAD_SIZE; ++i) {
    if (_payload[i] != 0xFF) { anyNotFF = true; break; }
  }
  _hdr.crc32 = anyNotFF ? crc32(_payload, REGISTRY_PAYLOAD_SIZE) : 0;
  return true;
}

bool EepromRegistry::find(uint16_t key, size_t& entryOff, size_t& entrySize) const {
  size_t off = 0;
  while (off + 4 <= REGISTRY_PAYLOAD_SIZE) {
    uint16_t k = (uint16_t)_payload[off] | ((uint16_t)_payload[off + 1] << 8);
    if (k == TLV_END_KEY || k == 0xFFFFu) break;

    const uint8_t type = _payload[off + 2];
    const uint8_t len  = _payload[off + 3];
    (void)type;

    const size_t total = 4u + (size_t)len;
    if (off + total > REGISTRY_PAYLOAD_SIZE) break;

    if (k == key) {
      entryOff = off;
      entrySize = total;
      return true;
    }
    off += total;
  }
  return false;
}

static bool tlv_append(uint8_t* payload, size_t payloadSize, uint16_t key, uint8_t type, const uint8_t* data, uint8_t len) {
  // Busca primer 0xFF 0xFF (fin implícito en payload vacío) o fin por TLV_END_KEY.
  size_t off = 0;
  while (off + 4 <= payloadSize) {
    uint16_t k = (uint16_t)payload[off] | ((uint16_t)payload[off + 1] << 8);
    if (k == 0xFFFFu) {
      break;
    }
    uint8_t l = payload[off + 3];
    size_t total = 4u + (size_t)l;
    if (off + total > payloadSize) return false;
    off += total;
  }

  const size_t need = 4u + (size_t)len;
  if (off + need > payloadSize) return false;

  payload[off]     = (uint8_t)(key & 0xFF);
  payload[off + 1] = (uint8_t)(key >> 8);
  payload[off + 2] = type;
  payload[off + 3] = len;

  for (uint8_t i = 0; i < len; ++i) payload[off + 4 + i] = data[i];

  // Marca fin (opcional, deja 0xFF si ya lo está). Es suficiente si el resto está 0xFF.
  if (off + need + 1 < payloadSize) {
    payload[off + need]     = 0xFF;
    payload[off + need + 1] = 0xFF;
  }
  return true;
}

bool EepromRegistry::remove(uint16_t key) {
  if (!_valid) return false;

  size_t off=0, sz=0;
  if (!find(key, off, sz)) return true; // nada que borrar

  // Compacta: mueve el resto hacia atrás y rellena con 0xFF
  const size_t tailOff = off + sz;
  const size_t tailLen = (tailOff <= REGISTRY_PAYLOAD_SIZE) ? (REGISTRY_PAYLOAD_SIZE - tailOff) : 0;

  memmove(&_payload[off], &_payload[tailOff], tailLen);
  memset(&_payload[REGISTRY_PAYLOAD_SIZE - sz], 0xFF, sz);

  return commit();
}

bool EepromRegistry::getU32(uint16_t key, uint32_t& out) const {
  size_t off=0, sz=0;
  if (!find(key, off, sz)) return false;
  const uint8_t type = _payload[off + 2];
  const uint8_t len  = _payload[off + 3];
  if (type != (uint8_t)TlvType::U32 || len != 4) return false;
  const uint8_t* v = &_payload[off + 4];
  out = (uint32_t)v[0] | ((uint32_t)v[1] << 8) | ((uint32_t)v[2] << 16) | ((uint32_t)v[3] << 24);
  return true;
}

bool EepromRegistry::getI32(uint16_t key, int32_t& out) const {
  uint32_t u=0;
  if (!getU32(key, u)) return false;
  out = (int32_t)u;
  return true;
}

bool EepromRegistry::getBool(uint16_t key, bool& out) const {
  size_t off=0, sz=0;
  if (!find(key, off, sz)) return false;
  const uint8_t type = _payload[off + 2];
  const uint8_t len  = _payload[off + 3];
  if (type != (uint8_t)TlvType::U8 || len != 1) return false;
  out = (_payload[off + 4] != 0);
  return true;
}

bool EepromRegistry::getStr(uint16_t key, char* out, size_t outLen) const {
  if (!out || outLen == 0) return false;
  size_t off=0, sz=0;
  if (!find(key, off, sz)) return false;
  const uint8_t type = _payload[off + 2];
  const uint8_t len  = _payload[off + 3];
  if (type != (uint8_t)TlvType::Str) return false;

  const size_t n = (len < (outLen - 1)) ? len : (outLen - 1);
  memcpy(out, &_payload[off + 4], n);
  out[n] = '\0';
  return true;
}

bool EepromRegistry::setU32(uint16_t key, uint32_t v) {
  if (!_valid) return false;
  remove(key); // simplificación (compacta) antes de reinsertar
  uint8_t buf[4] = {(uint8_t)v, (uint8_t)(v>>8), (uint8_t)(v>>16), (uint8_t)(v>>24)};
  if (!tlv_append(_payload, REGISTRY_PAYLOAD_SIZE, key, (uint8_t)TlvType::U32, buf, 4)) return false;
  return commit();
}

bool EepromRegistry::setI32(uint16_t key, int32_t v) {
  return setU32(key, (uint32_t)v);
}

bool EepromRegistry::setBool(uint16_t key, bool v) {
  if (!_valid) return false;
  remove(key);
  uint8_t b = v ? 1 : 0;
  if (!tlv_append(_payload, REGISTRY_PAYLOAD_SIZE, key, (uint8_t)TlvType::U8, &b, 1)) return false;
  return commit();
}

bool EepromRegistry::setStr(uint16_t key, const char* s) {
  if (!_valid) return false;
  if (!s) return false;

  const size_t len = strlen(s);
  if (len > 255) return false; // TLV len es u8

  remove(key);

  const uint8_t l = (uint8_t)len;
  if (!tlv_append(_payload, REGISTRY_PAYLOAD_SIZE, key, (uint8_t)TlvType::Str, (const uint8_t*)s, l)) return false;
  return commit();
}

} // namespace app::nvm
