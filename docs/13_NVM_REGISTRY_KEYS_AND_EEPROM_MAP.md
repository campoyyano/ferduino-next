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

Estado real (post B5.x): además de migración legacy (temperaturas/outlets/dosificación), ya existen keys de configuración de red/MQTT para el runtime (AppConfig).

### C.0 Configuración (AppConfig / network / MQTT)

Estas keys vienen de:
- `Firmware/port/include/app/config/app_config_keys.h`
- `Firmware/port/src/app/config/app_config.cpp`

> Nota: esta persistencia NO existe en el firmware original (legacy), donde broker/credenciales suelen ser compile-time.

| Key ID (u16) | Nombre estable | Tipo TLV | Semántica | Origen |
|---:|---|---|---|---|
| `200` | `mqtt.host` | `Str` | Hostname/IP del broker | Port (AppConfig) |
| `201` | `mqtt.port` | `U32` | Puerto broker MQTT | Port (AppConfig) |
| `204` | `mqtt.device_id` | `Str` | DeviceId (base de topics ferduino/<deviceId>/...) | Port (AppConfig) |
| `205` | `backend.mode` | `U32` | 0=Legacy, 1=HA | Port (AppConfig) |
| `210` | `net.dhcp` | `Bool` | true=DHCP, false=estática | Port (AppConfig) |
| `211` | `net.ip` | `U32` | IPv4 packed a\|b<<8\|c<<16\|d<<24 | Port (AppConfig) |
| `212` | `net.gw` | `U32` | Gateway IPv4 packed | Port (AppConfig) |
| `213` | `net.subnet` | `U32` | Subnet IPv4 packed | Port (AppConfig) |
| `214` | `net.dns` | `U32` | DNS IPv4 packed | Port (AppConfig) |
| `215` | `net.mac` | `Str` | MAC "02:FD:00:00:00:01" (17 chars) | Port (AppConfig) |

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


### C.2a Scheduler eventos (ventanas ON/OFF, 1..9)

Keys añadidas en Fase C1.2 para persistir una ventana ON/OFF por canal.
Dominio elegido: **3xx** (control de salidas) para evitar colisión con rango `600..699` reservado a RTC/tiempo.

| Key ID | Nombre estable | Tipo | Semántica | Origen | Estado |
|---:|---|---|---|---|---|
| `330` | `scheduler.events.valid` | `Bool` | true si existe config persistida | Port (C1.2) | Nuevo |
| `331..339` | `scheduler.events.chN.enabled` | `Bool` | habilita canal N (N=1..9) | Port (C1.2) | Nuevo |
| `340..348` | `scheduler.events.chN.onMinute` | `U32` | minuto del día ON (0..1439) | Port (C1.2) | Nuevo |
| `350..358` | `scheduler.events.chN.offMinute` | `U32` | minuto del día OFF (0..1439) | Port (C1.2) | Nuevo |

Notas:
- El motor vive en `Firmware/port/src/app/scheduler/app_event_scheduler.cpp`.
- `events::begin()` carga estas keys si `scheduler.events.valid=true`.
- `events::setWindow()` persiste cambios y hace commit agrupado (beginEdit/endEdit).

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
- `days_mask`: bit0=Lun, bit1=Mar, bit2=Mié, bit3=Jue, bit4=Vie, bit5=Sáb, bit6=Dom.

## D) Mapa EEPROM legacy (Original)

Fuente principal de offsets legacy:

- `Firmware/Original/Ferduino_Aquarium_Controller/src/Modules/Funcoes_EEPROM.h`
- Inicialización/lectura en setup:
  - `Firmware/Original/Ferduino_Aquarium_Controller/src/Modules/Setup.h`

### D.1 Network/MQTT (host/port/user/apikey/deviceId)

En el firmware original (legacy) **no se encontró persistencia EEPROM** para host/port/username/apikey/clientId.

- `Username` y `APIKEY` suelen ser compile-time: `Configuration.h`
- Broker y puerto suelen ser fijos en código (ej: `setServer("www.ferduino.com", 1883)`)

Conclusión:
- En legacy original, `network/mqtt` es mayormente estático (no EEPROM).
- En el port, `network/mqtt/backendMode` **sí** se persisten vía registry TLV (keys `200..215`) para permitir configuración sin recompilar (B6+).

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

- `100-199`: temps (migración legacy y sensores)
- `200-299`: network/mqtt/backend (AppConfig)
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
