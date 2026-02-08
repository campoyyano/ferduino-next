# ferduino-next — 07_CONTEXT.md (diario técnico y trazabilidad)

## B5.4 — Runtime skeleton (sin smoketests) + arranque mínimo real

Se introduce modo `SMOKETEST_NONE` para ejecutar un flujo runtime mínimo en vez de smoketests:

Orden de arranque:
1) `app::nvm::registry().begin()`
2) `app::nvm::migrateLegacyIfNeeded()`
3) `app::cfg::loadOrDefault()`
4) `hal::network().begin(cfg.net)`
5) `app::comms().begin()` (selección legacy/HA por config + init MQTT/callbacks)

Loop runtime:
- `hal::network().loop()`
- `app::comms().loop()`

Cambios clave:
- `include/app/app_smoketests.h`: se definen IDs numéricos de smoketests y `SMOKETEST_NONE`.
- Nuevo módulo runtime:
  - `include/app/runtime/app_runtime.h`
  - `src/app/runtime/app_runtime.cpp`
- `src/main.cpp`: ejecuta runtime si `SMOKETEST == SMOKETEST_NONE`.


## B5.3 — Migración legacy ampliada (Outlets + Dosificadora) → Registry TLV

Se amplía `app::nvm::migrateLegacyIfNeeded()` para copiar más estado desde EEPROM legacy (0..1023)
hacia el registry TLV (1024..4095), manteniendo idempotencia mediante `REGF_MIGRATED`.

### Outlets
- Legacy:
  - sentinel: `840` debe ser `66`
  - data: `841..849` (9 bytes)
- Registry:
  - `310 outlet.block_valid` (Bool)
  - `311..319 outlet.N.state` (U32)

### Dosificadora (6 canales)
- Legacy:
  - sentinel: `706` debe ser `66`
  - dosis personalizada: `545` (6 x int16 x10)
  - start hour: `557..562`
  - start min: `583..588`
  - días: `589..625` (segunda..domingo por canal)
  - qty: `631..636`
  - enabled: `637..642`
  - end hour: `643..648`
  - end min: `649..654`
- Registry:
  - `500 dosing.block_valid` (Bool)
  - `510..515 dosing.chN.dose_x10` (I32)
  - `520..525 start_hour` (U32)
  - `530..535 start_min` (U32)
  - `540..545 days_mask` (U32; bit0=Mon..bit6=Sun)
  - `550..555 quantity` (U32)
  - `560..565 enabled` (Bool)
  - `570..575 end_hour` (U32)
  - `580..585 end_min` (U32)

Docs actualizados: `docs/NVM_REGISTRY_KEYS_AND_EEPROM_MAP.md`.


## B5.2 — MQTT Admin (config runtime por MQTT, persistente en registry TLV)
Se implementa canal MQTT de administración:

- Topic comando: `ferduino/<deviceId>/cmd/config`
- Operaciones:
  - `get`/`GET`/`?` -> publica JSON de config actual en `ferduino/<deviceId>/cfg` (retained)
  - JSON set parcial -> aplica cambios en RAM, persiste con `app::cfg::save()` (registry TLV), responde ACK

Campos soportados en JSON:
- `backend`: `"legacy"` | `"ha"`
- `mqtt_host`, `mqtt_port`, `device_id`
- `net_dhcp`
- `net_ip`, `net_gw`, `net_subnet`, `net_dns` (strings a.b.c.d)
- `net_mac` (string "02:FD:00:00:00:01")

Notas:
- Si se modifica broker/deviceId/backend/red -> `reboot_required:true` (no reinicio automático).
- No se escribe en EEPROM legacy 0..1023; todo persiste vía registry TLV.


## Reglas de trabajo (acordadas)
- **Commits cortos y frecuentes**: cada paso cerrado (A9.x / Bx.y) → commit + actualización de este archivo.
- **Sin parches sueltos**: cuando se toca un archivo, se pega el fichero completo para evitar errores.
- **Compatibilidad legacy**: mantener firmware original en `Firmware/Original/` intacto.
- **Port (HAL + app mínima)** vive en `Firmware/port/`.
- **Zona EEPROM 0..1023**: se trata como **legacy read-only** (no escribir desde el port).

---

## Estado actual (alto nivel)

### A — HAL (port)
- A2 GPIO / smoketests: OK
- A3 Perfil de pines Ferduino: OK
- A4 PWM: OK
- A5 IO Expander PCF8575: OK
- A6 Serial/debug: OK
- A7 RTC: OK
- A8 Relays: OK
- A9 Ethernet + MQTT (Ethernet W5x00 + PubSubClient): OK (smoketests listos, sin placa aún)

### B — Comunicaciones y HA
- B1: Preparación comms facade + selección por `FERDUINO_COMMS_MODE`: OK
- B2: Backend legacy MQTT implementado (IDs 0..17) + glosario del protocolo: OK
- B3: Estructura HA (discovery/bridge/outlet control) y compilación: OK (sin hardware)

### B4 — NVM (EEPROM/registry)
Objetivo: permitir configuración persistente sin recompilar, manteniendo compatibilidad con EEPROM legacy del Ferduino original.

- B4.1: Definir filosofía NVM:
  - Registry TLV con header + payload + CRC
  - Magic/version/flags (REGF_MIGRATED)
  - Migración idempotente desde legacy
  - HAL de storage para poder migrar a FRAM/Flash en el futuro

- B4.2/B4.3: Layout EEPROM:
  - 0..1023 => LEGACY (solo lectura / migración)
  - 1024..4095 => REGISTRY TLV (RW)

---

## Paso actual en curso
### B5.1 — Consumir registry desde AppConfig (NO usar EEPROM offset 0)
Problema detectado:
- `app_config` escribía un bloque en EEPROM offset 0, rompiendo compatibilidad legacy.

Solución aplicada:
- `app_config` ahora guarda/carga por **keys TLV** vía `app::nvm::registry()`.
- Se añadió modo transacción `beginEdit/endEdit` para reducir commits.
- `main.cpp` inicializa:
  - `registry().begin()`
  - `migrateLegacyIfNeeded()`
  - `cfg::loadOrDefault()`

Keys TLV iniciales (AppConfig):
- 200 MQTT host (str)
- 201 MQTT port (u32)
- 204 device id (str)
- 205 backend mode (u32: 0 legacy / 1 HA)
- 210 DHCP (bool)
- 211 IP (u32 packed)
- 212 GW (u32 packed)
- 213 subnet (u32 packed)
- 214 DNS (u32 packed)
- 215 MAC (str "02:FD:00:00:00:01")

Notas:
- Migración legacy (subset inicial) a keys 100..102:
  - Temp set/off/alarm (décimas)
- Ampliar migración según se porten módulos del firmware original.

---

## Documentos generados / adjuntos al repo
- `AUDIT_CODEX_B4.md`: auditoría y checklist NVM
- `NVM_REGISTRY_KEYS_AND_EEPROM_MAP.md`: mapa de keys y layout EEPROM
- `PORTING_PLAN_B5_B6.md`: plan de portado de módulos del Ferduino original

---

## Próximos pasos
- B5.2: Extender `EepromMigration` con más variables legacy (según `Funcoes_EEPROM.h` del original).
- B6.x: Empezar portado incremental de módulos del Ferduino original usando flags y manteniendo build mínimo.
- Futuro hardware: evaluar FRAM pequeña para parámetros + FRAM grande/SD para logger.

### B4.4 – Integración de migración legacy → registry (subset mínimo)

**Objetivo**
Evitar pérdida de compatibilidad: si el registry nuevo no existe o aún no ha migrado, importar parámetros mínimos desde offsets legacy del firmware original.

**Implementación**
- Se añade `app::nvm::migrateLegacyIfNeeded()`:
  - Si `registry.isValid()==false` → `registry.format()` (inicializa header/payload)
  - Si `flags & REGF_MIGRATED` → no hace nada
  - Lee offsets legacy:
    - temp water set/off/alarm: 482 (int16), 484 (u8), 485 (u8)
  - Guarda en registry TLV keys (100..102) como enteros (valores en décimas x10)
  - Marca `REGF_MIGRATED` mediante `registry.setFlags(...)`

**Archivos**
- `include/app/nvm/eeprom_migration.h`
- `src/app/NVM/eeprom_migration.cpp`

**Estado**
- Compila correctamente y enlaza con `registry()` y `EepromRegistry::begin()`.
- Siguiente: ampliar migración a parámetros de red/MQTT y comenzar B5 (config runtime desde registry).


### B4.4 – Migración Legacy → Registry

Objetivo:
Permitir que dispositivos existentes con EEPROM del firmware original
se actualicen automáticamente al nuevo sistema.

Comportamiento:
- Si registry no existe → format()
- Si flag REGF_MIGRATED no está:
    - Leer parámetros críticos desde offsets legacy
    - Escribirlos como TLV en registry
    - Set flag REGF_MIGRATED

Migración inicial:
- Temperatura agua:
    offsets 482..485 (valores en décimas)

Impacto:
- Compatibilidad hacia atrás garantizada
- A partir de la migración el sistema usa solo registry nuevo
- Zona legacy permanece en modo solo lectura

Siguiente:
B5 – Inicio del uso del registry desde backends/configuración


### B4.3 – Registry TLV versionado (zona nueva)

**Objetivo**
Implementar un registro de configuración versionado sobre la zona nueva de EEPROM (1024..4095), desacoplado del layout legacy, usando `hal::storage()`.

**Implementación**
- Se añade `EepromRegistry` con formato TLV:
  - Entry: `[key:u16][type:u8][len:u8][value:len]`
  - Payload ocupa toda la zona `REGISTRY_PAYLOAD_SIZE` (tras el header)
- Header fijo (`RegistryHeader`) en `REGISTRY_BASE`:
  - `magic='FDNX'`, `version=1`, `flags`, `crc32`
- Validación:
  - Si `magic/version` no coinciden → registry no inicializado (no es error)
  - Si `crc32 != 0` → se valida contra `payload`
  - Si `crc32 == 0` → se considera payload vacío/skip CRC

**Archivos añadidos**
- `include/app/nvm/eeprom_registry.h`
- `src/app/nvm/eeprom_registry.cpp`
- (documentación legacy) `docs/eeprom_legacy_map.md`

**Notas**
- Diseño sin heap, apto para AVR.
- `Log/Archive` siguen pendientes (FRAM/SD) y se integrarán a través de `hal::storage()` cuando exista hardware.

**Siguiente**
- B4.4: detección legacy + migración mínima (solo claves críticas primero) y set de flag `REGF_MIGRATED`.


### B4.2 – Definición de layout dual de EEPROM (legacy + registry)

**Objetivo**
Congelar un mapa estable de EEPROM que:
- mantenga compatibilidad con el firmware Ferduino original (layout fijo),
- reserve una zona nueva para configuración versionada (registry TLV en B4.3),
- evite pisar offsets legacy.

**Decisión de layout (ATmega2560: 4096 bytes)**
Se divide la EEPROM en dos zonas:

| Zona | Rango | Uso | Política |
|------|------|-----|----------|
| Legacy (compat) | `0..1023` | offsets del firmware original | **RO** (no modificar) |
| Registry (nuevo) | `1024..4095` | header + payload TLV + CRC | **RW** |

Motivación:
- El firmware original usa offsets hasta ~871 (y LEDs en 1..~480).
- Reservamos 1KB completo (0..1023) para margen y estabilidad.
- El resto queda libre para el registry nuevo (3072 bytes).

**Archivos añadidos**
- `include/app/nvm/eeprom_layout.h`
  - Constantes de layout (bases, tamaños)
  - `RegistryHeader` con `magic/version/flags/crc32`
  - Magic definido: `'FDNX'` (`0x584E4446` little-endian)
  - Flag reservado: `REGF_MIGRATED` (se usará en B4.4)
- `docs/eeprom_map.md`
  - Documento de contrato del mapa dual (legacy + registry)
  - Referencia a la fuente legacy: `Modules/Funcoes_EEPROM.h` del firmware original

**Impacto**
- No hay cambios funcionales aún (solo contrato y documentación).
- Prepara B4.3: implementación del registry TLV sobre `hal::storage()` y este layout.


### B4.1 – Análisis de EEPROM del firmware original

**Objetivo**
Identificar el uso real de EEPROM en el firmware original Ferduino para:
- Mantener compatibilidad binaria.
- Definir el mapa de memoria persistente en ferduino-next.
- Preparar la futura HAL de almacenamiento (EEPROM / FRAM / Flash).

**Acciones realizadas**

1. Revisión del proyecto original:
   - Búsqueda de:
     - `EEPROM.read`
     - `EEPROM.write`
     - `EEPROM.get`
     - `EEPROM.put`
     - `writeAnything`
   - Identificación de estructuras persistidas completas.

2. Identificación de patrones de uso:
   - Uso de **bloques estructurados** (struct completos).
   - Direcciones base fijas (layout estático).
   - Inicialización por defecto cuando EEPROM no contiene valores válidos.
   - Dependencia de estos datos en:
     - Configuración de red
     - Parámetros del sistema
     - Calibraciones
     - Configuración de dispositivos

3. Conclusiones de arquitectura

- El firmware original **no usa key-value**, sino layout binario fijo.
- Para compatibilidad:
  
  ferduino-next debe:
  - Mantener offsets equivalentes.
  - Usar las mismas estructuras o equivalentes binarias.
  - Detectar EEPROM no inicializada.

4. Decisiones de diseño

Se define una capa de abstracción futura:

Storage HAL (NVM)
↓
EEPROM interna (Mega)
FRAM externa (futuro)
Flash emulada (MCUs sin EEPROM)

Estrategia:

- Layout fijo compatible con original
- Detección por **Magic Header**
- Soporte de migración futura

Magic propuesto:

Offset 0:
uint32_t magic = 0x4652444E // 'FRDN'
uint16_t version


Si no coincide:
→ cargar defaults
→ guardar estructura completa

5. Preparación para hardware futuro

Arquitectura preparada para:

- EEPROM interna (ATmega)
- FRAM pequeña → parámetros
- FRAM grande → data logger
- SD → almacenamiento histórico

Sin cambios funcionales aún en el código (solo análisis y definición de arquitectura).

**Estado**
- Análisis completado
- Compatibilidad definida
- Listo para B4.2 (definición del layout en ferduino-next)

### [2026-02-06] B3.4 – Administración de configuración por MQTT (GET/SET + persistencia)

- Se añade un canal de administración de configuración por MQTT:
  - Comando: `ferduino/<deviceId>/cmd/config`
  - Estado (retained): `ferduino/<deviceId>/cfg`
  - ACK: `ferduino/<deviceId>/cfg/ack`
- Payloads soportados:
  - `get` -> publica JSON con config actual.
  - JSON parcial -> aplica cambios y persiste en EEPROM (`cfg::save()`), responde con `reboot_required` cuando procede.
- Se añade `app::cfg::set(const AppConfig&)` para aplicar configuración en RAM antes de guardar.
- Integración:
  - `comms_ha_backend.cpp` y `comms_legacy_backend.cpp` se suscriben a `.../cmd/config` y delegan el parsing a `cfgadmin`.
  - El handler de cfgadmin se evalúa antes que el resto de comandos.
- Compilación verificada: **compila en mega2560_port**.

Estado: **B3.4 COMPLETADO**


### [2026-02-06] B3.3 – HA Discovery runtime (deviceId desde EEPROM)

- `ha_discovery` deja de depender de macros (`FERDUINO_DEVICE_ID`) y usa `app::cfg::get().mqtt.deviceId`.
- Topics de HA discovery quedan alineados con backend HA:
  - `ferduino/<deviceId>/state`
  - `ferduino/<deviceId>/availability`
  - `ferduino/<deviceId>/cmd/outlet/<n>` (switch outlets 1..9)
- `publishDiscoveryMinimal()/All()` publica configuración retained usando `device`/`uniq_id` derivados del `deviceId` runtime.
- Compilación verificada: **compila en mega2560_port**.

Estado: **B3.3 COMPLETADO**


### [2026-02-06] B3.2 – Selector runtime de backend + MQTT desde EEPROM (sin setup global)

- `app::comms()` deja de depender de `#if FERDUINO_COMMS_MODE` y selecciona backend en runtime usando:
  - `app::cfg::get().backendMode` (LEGACY / HA).
- Backend LEGACY:
  - MQTT `host/port/deviceId` pasan a leerse desde `app::cfg::get().mqtt.*` (EEPROM o defaults).
- Backend HA:
  - MQTT `host/port/deviceId` pasan a leerse desde `app::cfg::get().mqtt.*`.
  - `begin()` fuerza `app::cfg::loadOrDefault()` para no depender todavía de un `setup()` central (port aún sin entrypoint).
- Compilación verificada: **compila en mega2560_port**.

Nota: `ha_discovery` aún usa macros de deviceId; se migrará a runtime en el siguiente micro-paso.
Estado: **B3.2 COMPLETADO**


### [2026-02-06] B3.1 – Configuración runtime + EEPROM (base)

- Se introduce módulo `app::cfg` para configuración persistente:
  - Backend activo (LEGACY / HA)
  - Configuración MQTT (host, port, device_id)
  - Configuración de red (estructura preparada)
- Configuración versionada con:
  - magic (`FDNX`)
  - version
  - size
  - crc32
- Carga desde EEPROM con validación; fallback automático a defaults definidos por `build_flags`.
- API pública:
  - `loadOrDefault()`
  - `save()`
  - `factoryReset()`
  - `get()`
- Base preparada para eliminar `#if` de selección de backend en B3.2.
- Build verificado: **compila en mega2560_port**.

Estado: **B3.1 COMPLETADO**


### [2026-02-06] B2.3 – Control HA → Outlets (switch + cmd topics)

- Se actualiza Home Assistant Discovery: outlets 1..9 pasan de `binary_sensor` (read-only) a `switch` (controlables).
- Se definen topics de comando por outlet:
  - `ferduino/<device_id>/cmd/outlet/<n>` con payload `"1"` / `"0"`.
- Se implementa módulo de control de outlets (estado en RAM por ausencia de hardware):
  - `ha_outlet_control.*` mantiene `outlet_1..outlet_9` y aplica comandos.
  - Punto de integración preparado para mapear a HAL relés en fases posteriores.
- El backend HA:
  - se suscribe a `cmd/outlet/1..9`,
  - aplica cambios de outlet,
  - publica `ferduino/<device_id>/state` actualizado para reflejar el cambio inmediatamente.
- El bridge Legacy → HA prioriza el estado de outlets desde HA (simulado) para que el `state` publicado sea coherente con los comandos recibidos.
- Build verificado: **compila en mega2560_port**.

Estado: **B2.3 COMPLETADO**

### [2026-02-06] B2.2 – Bridge Legacy → Home Assistant (state + discovery)

- Se implementa el **bridge Legacy → HA**: a partir del JSON legacy del comando **ID0 (Home)** se genera un `state` HA derivado.
- Parseo AVR-friendly (sin parser JSON):
  - extracción por clave con `strstr` + `atof/atoi`
  - defaults a 0 si falta algún campo
  - no bloquea el loop ante JSON parcial/truncado
- Publicación del estado HA en:
  - `ferduino/<device_id>/state`
  con claves “HA-friendly” (snake_case):
  - `water_temperature`, `heatsink_temperature`, `ambient_temperature`
  - `water_ph`, `reactor_ph`, `orp`, `salinity`
  - `led_*_power`
  - `outlet_1..outlet_9`
  - `uptime`
- Hook del bridge integrado en `publishHome_ID0()` del backend legacy: cada publicación legacy ID0 genera también `state` HA.
- Se amplía **Home Assistant Discovery** para exponer entidades (retained):
  - Sensores de temperatura, química, LEDs y uptime.
  - Outlets 1..9 como `binary_sensor` (read-only) en esta fase.
- Se añade documentación de parseo y mapeo en:
  - `docs/ha_legacy_bridge_parsing.md`

Estado: **B2.2 COMPLETADO**


### [2026-02-06] A10 – Backend de comunicaciones Legacy (MQTT)

- Se implementa completamente el backend de comunicaciones **Legacy MQTT** compatible con Ferduino original.
- Selector de backend activo vía `FERDUINO_COMMS_MODE` (PlatformIO build_flags).
- Implementados todos los comandos legacy **ID 0..17**:
  - Parsing CSV con terminador `K`.
  - Distinción `mode == 0` (lectura) / `mode != 0` (escritura).
  - Respuestas JSON con **claves originales** del protocolo Ferduino.
  - Escrituras responden `{"response":"ok"}` según comportamiento legacy.
- Backend desacoplado mediante interfaz `ICommsBackend`.
- Integración con HAL MQTT (`PubSubClient`) y HAL Network.
- Código compila **sin warnings** en AVR (Mega2560).
- Sin dependencia de hardware real (va

### [2026-02-06] A10.1 – Selección de backend de comunicaciones (legacy vs HA)

- Se introduce selección de backend de comunicaciones por compilación mediante `FERDUINO_COMMS_MODE`.
- Se define la interfaz `ICommsBackend` y la factoría `app::comms()` para desacoplar la app del protocolo.
- Se preparan backends `legacy` y `HA` como stubs.
- En una fase posterior, el modo se persistirá en EEPROM para permitir configuración en runtime.

### [2026-02-06] A10.1 – Selección de backend de comunicaciones (legacy vs HA)

- Se define `CommsMode` y selección por compilación mediante `FERDUINO_COMMS_MODE` (PlatformIO `build_flags`).
- Se introduce la interfaz `ICommsBackend` y la factoría `app::comms()` para desacoplar la app del protocolo (legacy/HA).
- En una fase posterior, este modo se persistirá en EEPROM para ser configurable sin recompilar.


### [2026-02-06] A9.3.2 – Implementación MQTT (PubSubClient) + smoketest

- Implementación de `IMqttHal` para Mega2560 usando `PubSubClient` en `src/hal/impl/mega2560/`.
- MQTT se apoya en `hal::network().tcp()` mediante un wrapper mínimo compatible con `Client`.
- Se añade `app_mqtt_smoketest` para validar conexión al broker, publish y recepción de mensajes.
- `PubSubClient.h` y dependencias quedan limitadas a la implementación de plataforma.


### [2026-02-06] A9.3.1 – Definición de la HAL MQTT

- Se añade `Firmware/port/include/hal/hal_mqtt.h` con la interfaz pública `IMqttHal`.
- Se mantiene el desacoplo: el header no incluye librerías concretas (p. ej. `PubSubClient.h`).
- Se seguirá el patrón de acceso mediante `hal::mqtt()` (service locator) para evitar heap y simplificar integración.


### [2026-02-06] A9.2 – Implementación Ethernet (W5x00) + smoketest

- Se implementa `INetworkHal` para Mega2560 usando la librería `Ethernet` (W5x00) en `src/hal/impl/mega2560/`.
- Se añade `app_network_smoketest` para validar DHCP e impresión de IP (IP/GW/MASK/DNS).
- Regla mantenida: `Ethernet.h` solo aparece en la implementación específica de plataforma.


### [2026-02-06] A9.1 – Definición de la HAL de red

- Se introduce la interfaz pública `hal_network.h`.
- Se define el contrato de la HAL de red (Ethernet / futuros transports).
- No se incluyen dependencias de plataforma (`Ethernet.h`, etc.).
- La implementación se realizará en `src/hal/impl/<plataforma>/`.


## Estado actual del port (Firmware/port)

- El port es un proyecto PlatformIO limpio, desacoplado del firmware original de Ferduino.
- `Firmware/Original` se mantiene únicamente como referencia y **no debe incluirse** desde el port.
- Tras la reorganización de carpetas, el proyecto compila correctamente.

### Organización del código

- `Firmware/port/include/hal/`  
  Interfaces públicas de la HAL (sin includes dependientes de plataforma).

- `Firmware/port/src/hal/impl/mega2560/`  
  Implementaciones concretas de la HAL para Arduino Mega 2560.

- `Firmware/port/src/app/smoketests/`  
  Pruebas de integración simples para validar cada HAL de forma aislada.

- `Firmware/port/src/main.cpp`  
  Punto de entrada del firmware (selección de smoketests o lógica de aplicación).


## Objetivo general
Portar el firmware original del Ferduino (Arduino Mega 2560) a una arquitectura
moderna basada en HAL, manteniendo compatibilidad con el hardware original
antes de migrar a otras plataformas (Giga, ESP, etc).

La prioridad actual es:
1. Que compile
2. Que funcione en el hardware original
3. Que la abstracción HAL sea limpia y extensible

---

## Hardware actual objetivo
- Board principal: **Arduino Mega 2560**
- IO Expander: **PCF8575**
- RTC original: **DS1307**
- RTC alternativo soportado: **DS3231**
- LEDs PWM directos desde el Mega
- Relés controlados vía PCF8575

---

## Decisiones de diseño importantes

### HAL
- Todo acceso a hardware pasa por HAL
- No se accede directamente a Wire / digitalWrite / pinMode desde la app
- HAL es C++ ligero, sin dependencias innecesarias

### IO Expander
- Se usa un HAL genérico (`IoExpander`) con:
  - Dirección configurable
  - Soporte ACTIVE_LOW
  - Shadow register interno
- Implementación basada en I2C puro (no dependencia directa de librerías externas)

### RTC
- HAL RTC soporta **DS1307 y DS3231**
- API unificada (`FechaHora`)
- Selección por disponibilidad en runtime
- RTC ya compilando correctamente

---

## Estado actual (cerrado)

### HAL completados
- hal_gpio
- hal_time
- hal_i2c
- hal_pwm
- hal_ioexpander (PCF8575)
- hal_rtc (DS1307 + DS3231)

### Smoke tests existentes y compilando
- app_gpio_smoketest
- app_ferduino_pins_smoketest
- app_ioexpander_smoketest
- app_pwm_leds_smoketest
- app_rtc_smoketest

---

## Pendiente / siguientes pasos

### HAL pendientes
- hal_relay (encima de IoExpander)
- hal_network (Ethernet / MQTT) – NO prioritario aún

### A corto plazo
1. Implementar `hal_relay`
2. Smoke test de relés
3. Validar en hardware real (Mega original)
4. Limpieza final de includes y dependencias

---

## Notas importantes
- El repositorio es la fuente de verdad
- No se deben subir ZIPs ni `.pio`
- Archivos grandes NUNCA al repo (GitHub limit)
