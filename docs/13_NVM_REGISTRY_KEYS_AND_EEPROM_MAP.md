# NVM Registry Keys and EEPROM Map

- Fecha: 2026-02-08
- Commit: (actualiza al hacer commit)
- Alcance: documentación de `Firmware/port` + referencia read-only de `Firmware/Original`

## A) Overview (NVM architecture)

Arquitectura actual (port):

- Layout dual EEPROM (`Firmware/port/include/app/nvm/eeprom_layout.h`):
  - Legacy (RO): `0..1023`
  - Registry TLV (RW): `1024..4095`
- Header registry:
  - `REGISTRY_HEADER_OFFSET = 1024`
  - `REGISTRY_PAYLOAD_OFFSET = 1024 + sizeof(RegistryHeader)`
  - `REGISTRY_PAYLOAD_SIZE = 3072 - sizeof(RegistryHeader)`
- Header fields (`RegistryHeader`):
  - `magic` (`REGISTRY_MAGIC = 0x584E4446`, "FDNX")
  - `version` (`REGISTRY_VERSION = 1`)
  - `flags`
  - `crc32`

Semántica CRC y migración:

- En `Firmware/port/src/app/nvm/eeprom_registry.cpp`:
  - `crc32 == 0` significa payload vacío/no inicializado (se acepta como válido).
  - `crc32 != 0` obliga validación CRC del payload completo.
- Flag `REGF_MIGRATED` (`0x0001`):
  - Marca que la migración legacy -> registry ya fue ejecutada.
  - Evita remigración en cada arranque.

## B) Tabla de FLAGS (registry)

| Flag | Valor | Significado | Dónde se setea | Condición |
|---|---:|---|---|---|
| `REGF_NONE` | `0x0000` | Sin flags activas | `Firmware/port/src/app/nvm/eeprom_registry.cpp` (`EepromRegistry::format`) | Inicialización/formateo del registry |
| `REGF_MIGRATED` | `0x0001` | Migración legacy completada | `Firmware/port/src/app/nvm/eeprom_migration.cpp` (`migrateLegacyIfNeeded`) | Tras copiar legacy y hacer `setFlags(...)` |

## C) Tabla de KEYS del registry (actuales)

Estado real (post B5.3): se añaden keys para outlets + dosificación además de temperatura.

### C.1 Temperatura (agua)

| Key ID (u16) | Nombre estable | Tipo TLV | Unidad/escala | Origen legacy | Estado |
|---:|---|---|---|---|---|
| `100` | `temp.water.set_x10` | `I32` | décimas °C (`x10`) | `482 (int16)` | Migrado |
| `101` | `temp.water.off_x10` | `I32` | décimas °C (`x10`) | `484 (u8)` | Migrado |
| `102` | `temp.water.alarm_x10` | `I32` | décimas °C (`x10`) | `485 (u8)` | Migrado |

### C.2 Outlets (1..9)

| Key ID | Nombre estable | Tipo | Semántica | Origen legacy | Estado |
|---:|---|---|---|---|---|
| `310` | `outlet.block_valid` | `Bool` | true si legacy `840==66` | `840 (u8 sentinel)` | Migrado |
| `311..319` | `outlet.N.state` | `U32` | estado legacy (0/1 habitual) | `841..849 (9 bytes)` | Migrado |

### C.3 Dosificadora (6 canales)

| Key ID | Nombre estable | Tipo | Semántica | Origen legacy | Estado |
|---:|---|---|---|---|---|
| `500` | `dosing.block_valid` | `Bool` | true si legacy `706==66` | `706 (u8 sentinel)` | Migrado |
| `510..515` | `dosing.chN.dose_x10` | `I32` | dosis personalizada x10 | `545 (6 x int16)` | Migrado |
| `520..525` | `dosing.chN.start_hour` | `U32` | hora inicio | `557..562` | Migrado |
| `530..535` | `dosing.chN.start_min` | `U32` | minuto inicio | `583..588` | Migrado |
| `540..545` | `dosing.chN.days_mask` | `U32` | bits Mon..Sun (0..6) | `589..625` | Migrado |
| `550..555` | `dosing.chN.quantity` | `U32` | cantidad dosis | `631..636` | Migrado |
| `560..565` | `dosing.chN.enabled` | `Bool` | canal activo | `637..642` | Migrado |
| `570..575` | `dosing.chN.end_hour` | `U32` | hora fin | `643..648` | Migrado |
| `580..585` | `dosing.chN.end_min` | `U32` | minuto fin | `649..654` | Migrado |

Notas:
- `days_mask`: bit0=Mon(segunda), bit1=Tue(terca), bit2=Wed(quarta), bit3=Thu(quinta), bit4=Fri(sexta), bit5=Sat(sabado), bit6=Sun(domingo).

## D) Mapa EEPROM legacy (Original)

Fuente principal de offsets legacy:

- `Firmware/Original/Ferduino_Aquarium_Controller/src/Modules/Funcoes_EEPROM.h`
- Inicialización/lectura en setup:
  - `Firmware/Original/Ferduino_Aquarium_Controller/src/Modules/Setup.h`

### D.1 Network/MQTT (host/port/user/apikey/deviceId)

No se encontró persistencia EEPROM para host/port/username/apikey/clientId en el original.

- `Username` y `APIKEY` están en compile-time: `Configuration.h`
- Broker y puerto fijos en setup (`MQTT.setServer("www.ferduino.com", 1883)`)

Conclusión: en legacy original, `network/mqtt` es mayormente estático (no EEPROM); en el port se migra a registry.

### D.2 Offsets relevantes (subset migrado)

| Dirección legacy | Tamaño/tipo | Símbolo/función original | Uso | Key registry | Estado |
|---:|---|---|---|---|---|
| `482` | `int16` | `SaveTempToEEPROM` | temp set x10 | `100` | Migrado |
| `484` | `u8` | `SaveTempToEEPROM` | temp off x10 | `101` | Migrado |
| `485` | `u8` | `SaveTempToEEPROM` | temp alarm x10 | `102` | Migrado |
| `840` | `u8 sentinel` | `salvar_outlets_EEPROM` | bloque outlets válido | `310` | Migrado |
| `841..849` | `9 x u8` | `salvar_outlets_EEPROM` | outlets[1..9] | `311..319` | Migrado |
| `706` | `u8 sentinel` | `Salvar_dosadora_EEPROM` | bloque dosing válido | `500` | Migrado |
| `545` | `6 x int16` | `Salvar_dosadora_EEPROM` | dose personalizada x10 | `510..515` | Migrado |
| `557..562` | `6 x u8` | `Salvar_dosadora_EEPROM` | start hour | `520..525` | Migrado |
| `583..588` | `6 x u8` | `Salvar_dosadora_EEPROM` | start min | `530..535` | Migrado |
| `589..625` | `7 x 6 x u8` | `Salvar_dosadora_EEPROM` | días activos | `540..545` | Migrado |
| `631..636` | `6 x u8` | `Salvar_dosadora_EEPROM` | quantity | `550..555` | Migrado |
| `637..642` | `6 x u8` | `Salvar_dosadora_EEPROM` | enabled | `560..565` | Migrado |
| `643..648` | `6 x u8` | `Salvar_dosadora_EEPROM` | end hour | `570..575` | Migrado |
| `649..654` | `6 x u8` | `Salvar_dosadora_EEPROM` | end min | `580..585` | Migrado |

## E) Reglas de asignación de keys (política)

Rangos reservados por dominio (propuesto):

- `100-199`: temps
- `200-299`: mqtt/network
- `300-399`: relays/outlets
- `400-499`: pwm/led
- `500-599`: dosing
- `600-699`: rtc/time
- `700-799`: UI
- `800-899`: logger

Convención:
- Nunca reciclar IDs existentes con semántica distinta.
- Si cambia semántica/tipo: crear key nueva y mantener compatibilidad temporal.
- Migradores idempotentes (por flags o por versión).
- Actualizar siempre este documento al añadir keys.
