# NVM Registry Keys and EEPROM Map

- Fecha: 2026-02-08
- Commit: `b29ef64`
- Alcance: documentación de `Firmware/port` + referencia read-only de `Firmware/Original`

## Comandos usados durante el análisis

```powershell
git rev-parse --short HEAD
Get-Date -Format "yyyy-MM-dd"
rg -n "REGF_|REGISTRY_|KEY_|setI32\(|setU32\(|setStr\(|setBool\(|getI32\(|getU32\(|migrateLegacyIfNeeded|EEPROM_ADDR" Firmware/port/include Firmware/port/src
rg -n "EEPROM|writeAnything|readAnything|mqtt|broker|api|apikey|username|host|port|device|temp|set|off|alarm|outlet|dosa|dose|schedule|timer" Firmware/Original/Ferduino_Aquarium_Controller/src/Modules Firmware/Original/Ferduino_Aquarium_Controller/Configuration.h
Get-Content Firmware/Original/Ferduino_Aquarium_Controller/src/Modules/Funcoes_EEPROM.h
Get-Content Firmware/Original/Ferduino_Aquarium_Controller/src/Modules/Setup.h
Get-Content Firmware/Original/Ferduino_Aquarium_Controller/src/Modules/Funcoes_ESP8266.h
Get-Content Firmware/Original/Ferduino_Aquarium_Controller/Configuration.h
rg -n "BROKER|MQTT|USERNAME|APIKEY|topic/command|topic/response|setServer|connect\(|subscribe\(|publish\(" Firmware/Original/Ferduino_Aquarium_Controller/src/Modules Firmware/Original/Ferduino_Aquarium_Controller/Configuration.h Firmware/Original/Ferduino_Aquarium_Controller/src/Ferduino_Aquarium_Controller.cpp
```

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
| `REGF_MIGRATED` | `0x0001` | Migración legacy completada | `Firmware/port/src/app/nvm/eeprom_migration.cpp` (`migrateLegacyIfNeeded`) | Tras copiar subset legacy y hacer `setFlags(...)` |

Notas:

- Lectura de flags: `EepromRegistry::flags()` en `Firmware/port/include/app/nvm/eeprom_registry.h`.
- Escritura de flags: `EepromRegistry::setFlags(...)` + `commit()`.

## C) Tabla de KEYS del registry (actuales)

Estado real a `b29ef64`: solo hay 3 keys concretas definidas/insertadas en el port.

| Key ID (u16) | Nombre sugerido estable | Tipo TLV | Unidad/escala | Default/fallback | Consumidor (archivo + función) | Productor (archivo + función) | Notas |
|---:|---|---|---|---|---|---|---|
| `100` | `temp.water.set_x10` | `I32` | décimas °C (`x10`) | Si falla lectura legacy: `0` | Sin consumidor explícito aún en app/comms (pendiente B5/B6) | `Firmware/port/src/app/nvm/eeprom_migration.cpp` (`migrateLegacyIfNeeded` -> `reg.setI32`) | Valor viene de legacy `int16` en offset 482 |
| `101` | `temp.water.off_x10` | `I32` | décimas °C (`x10`) | Si falla lectura legacy: `0` | Sin consumidor explícito aún en app/comms | `Firmware/port/src/app/nvm/eeprom_migration.cpp` (`migrateLegacyIfNeeded` -> `reg.setI32`) | Legacy original era `byte` (offset 484) y se normaliza a `I32` |
| `102` | `temp.water.alarm_x10` | `I32` | décimas °C (`x10`) | Si falla lectura legacy: `0` | Sin consumidor explícito aún en app/comms | `Firmware/port/src/app/nvm/eeprom_migration.cpp` (`migrateLegacyIfNeeded` -> `reg.setI32`) | Legacy original era `byte` (offset 485) y se normaliza a `I32` |

Notas de implementación TLV actuales:

- Tipos soportados: `U8/U16/U32/I32/Blob/Str` (`Firmware/port/include/app/nvm/eeprom_registry.h`).
- `Str` guarda sin `\0`; longitud máxima actual: `255` bytes (`len` TLV es `u8`).
- API `get*`/`set*` existe, pero fuera de migración todavía no hay uso sistemático de keys en módulos funcionales.

## D) Mapa EEPROM legacy (Original)

Fuente principal de offsets legacy:

- `Firmware/Original/Ferduino_Aquarium_Controller/src/Modules/Funcoes_EEPROM.h`
- Inicialización/lectura en setup:
  - `Firmware/Original/Ferduino_Aquarium_Controller/src/Modules/Setup.h`

### D.1 Network/MQTT (host/port/user/apikey/deviceId)

No se encontró persistencia EEPROM para host/port/username/apikey/clientId en el original.

- `Username` y `APIKEY` están en compile-time:
  - `Firmware/Original/Ferduino_Aquarium_Controller/Configuration.h` (líneas ~197-198)
- Broker y puerto fijos en setup Ethernet:
  - `Firmware/Original/Ferduino_Aquarium_Controller/src/Modules/Setup.h` (`MQTT.setServer("www.ferduino.com", 1883)`)
- Topics legacy se construyen en runtime con `Username/APIKEY`:
  - `Firmware/Original/Ferduino_Aquarium_Controller/src/Modules/Funcoes_ESP8266.h`
  - `Firmware/Original/Ferduino_Aquarium_Controller/src/Modules/Webserver.h`

Conclusión: en legacy original, `network/mqtt` es mayormente estático (no EEPROM); en el port se recomienda migrarlo a keys registry.

### D.2 Offsets relevantes (temperaturas, relés/salidas, dosing/schedules)

| Dirección legacy | Tamaño/tipo | Símbolo/función original | Uso | Key registry recomendada | Estado |
|---:|---|---|---|---|---|
| `482` | `int16` (vía struct) | `SaveTempToEEPROM` / `ReadFromEEPROM` (`tempSettings.tempset`) | Setpoint temp agua `x10` | `100 temp.water.set_x10` | Migrado ya |
| `484` | `byte` (vía struct) | `SaveTempToEEPROM` / `ReadFromEEPROM` (`tempSettings.tempoff`) | Offset/apagado temp agua `x10` | `101 temp.water.off_x10` | Migrado ya |
| `485` | `byte` (vía struct) | `SaveTempToEEPROM` / `ReadFromEEPROM` (`tempSettings.tempalarm`) | Alarma temp agua `x10` | `102 temp.water.alarm_x10` | Migrado ya |
| `840` | `byte` sentinel | `salvar_outlets_EEPROM` / `Setup.h` | Marca bloque outlets válido | `310 outlet.block_valid` (bool) | Pendiente |
| `841..849` | `9 x byte` | `salvar_outlets_EEPROM` / `ler_outlets_EEPROM` | Estado outlets 1..9 | `311..319 outlet.N.state` | Pendiente |
| `706` | `byte` sentinel | `Salvar_dosadora_EEPROM` / `ler_dosadora_EEPROM` | Marca bloque dosificación válido | `500 dosing.block_valid` | Pendiente |
| `545` | `array struct` (`dosaconfig[6]`) | `Salvar_dosadora_EEPROM` / `ler_dosadora_EEPROM` | Dosis personalizada por canal (`x10`) | `510..515 dosing.chN.dose_x10` | Pendiente |
| `557..562` | `6 x byte` | `Salvar_dosadora_EEPROM` | Hora inicio dosificación canal | `520..525 dosing.chN.start_hour` | Pendiente |
| `583..588` | `6 x byte` | `Salvar_dosadora_EEPROM` | Minuto inicio dosificación canal | `530..535 dosing.chN.start_min` | Pendiente |
| `589..625` | `7 x 6 bytes` | `Salvar_dosadora_EEPROM` | Días activos dosificación canal | `540..545 + mask` (bitmask sugerido) | Pendiente |
| `631..636` | `6 x byte` | `Salvar_dosadora_EEPROM` | Cantidad dosis por canal | `550..555 dosing.chN.quantity` | Pendiente |
| `637..642` | `6 x byte` | `Salvar_dosadora_EEPROM` | Modo on/off dosificación canal | `560..565 dosing.chN.enabled` | Pendiente |
| `643..648` | `6 x byte` | `Salvar_dosadora_EEPROM` | Hora fin dosificación canal | `570..575 dosing.chN.end_hour` | Pendiente |
| `649..654` | `6 x byte` | `Salvar_dosadora_EEPROM` | Minuto fin dosificación canal | `580..585 dosing.chN.end_min` | Pendiente |
| `508..510` | `3 x byte` | `SavePHRToEEPROM` / `lerPHREEPROM` | Reactor pH set/off/alarm (`x10`) | `140..142` (temps/química) o `620..622` (time/control) | Pendiente |
| `520..522` | `3 x byte` | `SavePHAToEEPROM` / `lerPHAEEPROM` | Acuario pH set/off/alarm (`x10`) | `130..132` | Pendiente |
| `514` (+struct) | struct `config_ORP` | `SaveORPToEEPROM` / `lerORPEEPROM` | ORP set/off/alarm | `150..152` | Pendiente |

Notas:

- Además existen muchos offsets legacy de LEDs, clima, alimentador, timers y Dallas addresses en el mismo archivo.
- Formateo legacy original usa `EEPROM[0]` como marcador de formato (`Setup.h`), no parte del contrato TLV del port.

## E) Tabla de equivalencias (Legacy -> Registry)

Resumen operativo (subset priorizado):

| legacy_offset | legacy_name | registry_key | status |
|---:|---|---|---|
| `482` | `tempSettings.tempset` | `100 temp.water.set_x10` | Migrado ya |
| `484` | `tempSettings.tempoff` | `101 temp.water.off_x10` | Migrado ya |
| `485` | `tempSettings.tempalarm` | `102 temp.water.alarm_x10` | Migrado ya |
| `840` | outlets sentinel | `310 outlet.block_valid` | Pendiente |
| `841..849` | outlets[1..9] | `311..319 outlet.N.state` | Pendiente |
| `545` | dosaconfig[6].dose_personalizada | `510..515 dosing.chN.dose_x10` | Pendiente |
| `557..562` | dosing start hour | `520..525 dosing.chN.start_hour` | Pendiente |
| `583..588` | dosing start minute | `530..535 dosing.chN.start_min` | Pendiente |
| `637..642` | dosing enable | `560..565 dosing.chN.enabled` | Pendiente |
| `643..648` | dosing end hour | `570..575 dosing.chN.end_hour` | Pendiente |
| `649..654` | dosing end minute | `580..585 dosing.chN.end_min` | Pendiente |

## F) Reglas de asignación de keys (política futura)

Rangos reservados por dominio (propuesto):

- `100-199`: temps
- `200-299`: mqtt/network
- `300-399`: relays/outlets
- `400-499`: pwm/led
- `500-599`: dosing
- `600-699`: rtc/time
- `700-799`: UI
- `800-899`: logger

Convención de nombres:

- Formato: `dominio.subdominio.variable[_unidad]`
- Ejemplos:
  - `temp.water.set_x10`
  - `mqtt.port`
  - `outlet.1.state`
  - `dosing.ch1.start_hour`

Convención de tipos:

- `Bool/U8/U16/U32` para flags y enteros naturales.
- `I32` para valores con signo o escalados legacy (p.ej. `x10`).
- `Str` para host/deviceId (validar longitud <=255).
- `Blob` para MAC/IP empaquetados cuando convenga.

Versionado sin romper compatibilidad:

1. Nunca reciclar IDs existentes con semántica distinta.
2. Si cambia semántica/tipo: crear key nueva y mantener compatibilidad de lectura temporal.
3. Mantener migradores idempotentes por versión (`REGF_*` o key de versión de migración).
4. Actualizar siempre este documento con cualquier alta/baja/cambio de key.

## G) Checklist de implementación

Para añadir una key nueva:

1. Definir `key ID` + `tipo TLV` + nombre estable.
2. Añadir lectura con fallback (build_flags/defaults) en config/model.
3. Añadir escritura (cuando corresponda) y persistencia con `commit`.
4. Añadir soporte en canal admin MQTT `set/get` (si aplica).
5. Añadir migración desde legacy offset (si aplica).
6. Añadir pruebas/smoketest de ida/vuelta sin placa.
7. Actualizar este documento:
   - tabla de keys
   - equivalencias legacy->registry
   - estado (migrado/pendiente)

---

## Top 10 keys que faltan (prioridad B5/B6)

1. `200 mqtt.host` (`Str`) - crítica para backend runtime.
2. `201 mqtt.port` (`U16`/`U32`) - crítica para conexión broker.
3. `202 mqtt.device_id` (`Str`) - crítica para topics/HA discovery.
4. `210 net.use_dhcp` (`Bool`) - base de red runtime.
5. `211 net.mac` (`Blob[6]`) - identidad de interfaz Ethernet.
6. `212 net.ipv4` (`Blob[4]`) - modo estático.
7. `213 net.gateway` (`Blob[4]`) - routing estático.
8. `214 net.subnet` (`Blob[4]`) - máscara estática.
9. `300 outlet.1_9.state` (`U8` o `Bool` por key) - sincronización control legacy/HA.
10. `500 dosing.ch1_6.core` (`Struct split en keys U8/U16/I32`) - scheduling y dosificación (bloque funcional mayor de B6).
