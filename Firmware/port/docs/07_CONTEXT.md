# ferduino-next — 07_CONTEXT.md (diario técnico y trazabilidad)

## C2.1b — PORTING_TRACE y flags de habilitación

- Se crea `docs/PORTING_TRACE.md` como matriz de trazabilidad Original → Port:
  - Registra origen/destino, inputs/outputs, NVM keys, topics MQTT/HA y diferencias de comportamiento.
  - Incluye explícitamente flags de habilitación por módulo (debug/testing) y flags de backend FAKE/REAL.
- Política nueva:
  - Toda unidad portadas llevará comentarios `[PORT]` en el código con src/dst/nvm/mqtt/flags/diffs.
- Decisión de documentación:
  - Se recomienda que el contexto operativo (`07_CONTEXT.md`) sea canónico dentro de `port/docs/` para auditoría y consistencia.


## C2.1b — Telemetría MQTT tempctrl + trazabilidad de port

- Se añade telemetría MQTT para el control de temperatura (`tempctrl`):
  - Topic: `ferduino/<deviceId>/telemetry/tempctrl` (no retained)
  - Payload: JSON con `water_x10`, `set/off/alarm_x10`, `heater_on`, `chiller_on`, `alarm_active`, `valid`
  - Periodo actual: 10s (en `app_runtime.cpp`)
- Se crea documento de trazabilidad del port:
  - `docs/PORTING_TRACE.md` (matriz Original → Port)
  - Se adopta convención de comentarios `[PORT]` en el código para auditoría automática.

## C2.1 — Fix integración temp_control con sensores actuales

- Se corrige `app/temp_control` para adaptarse a la API real de `app::sensors::Temperatures`:
  - Sustituye `temps.valid` por `temps.water_valid`
  - Sustituye `temps.water_x10` por conversión `water_c -> x10`
- Se mantiene el desacoplo de hardware mediante `APP_TEMPCTRL_USE_GPIO` (0 por defecto).


## C2.1 — Port de control de temperatura (lógica) + cero duplicados de headers

- Se crea módulo `app/temp_control` que porta la lógica de control de temperatura del proyecto original (heater/chiller):
  - Lee `set/off/alarm` desde NVM registry (keys `100..102`, x10).
  - Usa la lectura de agua desde `app/sensors` (FAKE por defecto).
  - Calcula `heater_on`, `chiller_on` y `alarm_active`, con banda muerta y límites de seguridad (10°C..50°C).
  - Evita heater/chiller activos simultáneamente.
- El control está desacoplado del hardware:
  - Flag nuevo `APP_TEMPCTRL_USE_GPIO` (0 por defecto) para activar escritura real a GPIO (`aquecedorPin=42`, `chillerPin=43`) cuando haya hardware.
- Integración en `app/runtime/app_runtime.cpp`:
  - `sensors::begin/loop`
  - `tempctrl::begin/loop`
- Limpieza de duplicidades:
  - Se elimina `src/app/outlets/app_outlets.h` para evitar ambigüedad de includes (fuente única en `include/app/outlets/app_outlets.h`).


## C2 — Limpieza de duplicados de headers

- Se elimina duplicidad detectada de `app_outlets.h` para evitar ambigüedad de include paths en PlatformIO:
  - Se mantiene como fuente única: `include/app/outlets/app_outlets.h`
  - Se elimina: `src/app/outlets/app_outlets.h`
- Esto evita “bugs fantasma” cuando dos headers con el mismo nombre divergen y el compilador incluye uno u otro según orden de rutas.


## C1.2 + C2 — Schedule persistente + arbitraje Auto/Manual aplicado a Outlets

- Se añade persistencia TLV del motor de eventos (scheduler windows):
  - Keys: `330 scheduler.events.valid`
  - `331..339` enabled por canal
  - `340..348` onMinute por canal (U32)
  - `350..358` offMinute por canal (U32)
- Se añade comando MQTT HA backend para configurar schedule por canal:
  - Topic: `ferduino/<deviceId>/cmd/schedule/<1..9>`
  - Payload JSON minimal: `{"enabled":1,"on":"HH:MM","off":"HH:MM"}` (campos opcionales)
- Se implementa arbitraje Auto/Manual en outlets:
  - Manual (`cmd/outlet/<n>`) fuerza `auto=false` para ese canal
  - Schedule `enabled=true` fuerza `auto=true`
  - Schedule `enabled=false` explícito fuerza `auto=false`
  - Persistencia auto por canal en TLV:
    - `320 outlet.mode_block_valid`
    - `321..329 outlet.N.auto`
- Integración en runtime:
  - Se aplica `desiredOn` del scheduler a cada outlet **solo si** está en modo auto.
  - No hay sobrescritura del usuario cuando el canal está en manual.

## C1.2 — Scheduler eventos: persistencia TLV + admin MQTT (HA/Legacy)

- Se añade persistencia de ventanas ON/OFF del scheduler de eventos en `app::nvm::EepromRegistry` (TLV):
  - Keys (dominio 3xx relays/outlets):
    - `330`: `scheduler.events.valid` (bool)
    - `331..339`: `scheduler.events.chN.enabled` (bool)
    - `340..348`: `scheduler.events.chN.onMinute` (u32, 0..1439)
    - `350..358`: `scheduler.events.chN.offMinute` (u32, 0..1439)
  - `events::begin()` carga desde NVM si `valid=true`.
  - `events::setWindow()` persiste cambios (commit agrupado con beginEdit/endEdit).

- Se añade configuración por MQTT (sin ArduinoJson) en ambos backends:
  - Cmd: `ferduino/<deviceId>/cmd/schedule/<N>` (N=1..9)
  - Payload JSON: `{"on":"HH:MM","off":"HH:MM","enabled":true}`
    - Si no viene `enabled` pero sí horarios → `enabled=true`
    - Si falta `on` o `off` → se conserva el valor actual
  - Retained cfg: `ferduino/<deviceId>/cfg/schedule/<N>`
    - `{"ch":N,"enabled":0|1,"on":<min>,"off":<min>}`
  - Ack: `ferduino/<deviceId>/cfg/schedule/ack` (no retained)
    - `{"ok":true,"ch":N}`

- Se actualiza `docs/NVM_REGISTRY_KEYS_AND_EEPROM_MAP.md` con las nuevas keys.

## C1.1 — Scheduler de eventos (núcleo) + fix de compatibilidad con runtime/comms

- Se corrige `app_scheduler.cpp` para que implemente exactamente la API declarada en `include/app/scheduler/app_scheduler.h`:
  - `now()`, `minutesSinceBoot()`, `minuteTick()/consumeMinuteTick()`, `timeSource()`
  - Se mantiene soporte RTC opcional vía hook `app_scheduler_rtc_minute_of_day()` y fallback a millis.

- Se añade motor mínimo de eventos (Fase C1) sin hardware:
  - Nuevo API: `include/app/scheduler/app_event_scheduler.h`
  - Implementación: `src/app/scheduler/app_event_scheduler.cpp`
  - 1 ventana ON/OFF por canal (9 canales), soporta cruce de medianoche.
  - Evalúa solo en tick de minuto (`consumeMinuteTick()`), mantiene estado “desired” por canal.

- Integración respetando arquitectura actual del port:
  - `src/app/runtime/app_runtime.cpp` integra `events::begin()` y `events::loop()` sin tocar la gestión MQTT del backend (`app::comms()`).
  - `src/app/runtime/app_telemetry.cpp` publica telemetría debug mínima:
    - `ferduino/<deviceId>/telemetry/event_scheduler` (canal 0: enabled/on/off/desired).


## C1.1 — Núcleo de scheduler de eventos (ventana ON/OFF) + fix API scheduler base

- Se corrige el bloqueante pre-C: `app_scheduler` tenía API inconsistente entre header y `.cpp`.
  - `src/app/scheduler/app_scheduler.cpp` ahora implementa lo declarado en `include/app/scheduler/app_scheduler.h`:
    - `now()`, `minutesSinceBoot()`, `minuteTick()/consumeMinuteTick()`, `timeSource()`
  - Esto desbloquea consumidores como `app_runtime/app_telemetry` y reduce riesgo de link/undefined refs.

- Se añade motor mínimo de eventos (Fase C1) sin hardware:
  - Nuevo API: `include/app/scheduler/app_event_scheduler.h`
  - Implementación: `src/app/scheduler/app_event_scheduler.cpp`
  - Modelo actual: 1 ventana ON/OFF por canal (9 canales), soporta cruce medianoche.
  - El motor se evalúa únicamente en tick de minuto (`consumeMinuteTick()`), con estado “deseado” por canal.

- Integración suave (sin controlar outlets todavía):
  - `app_runtime.cpp`: `events::begin()` y `events::loop()`.
  - `app_telemetry.cpp`: se publica telemetría debug mínima:
    - `ferduino/<deviceId>/telemetry/event_scheduler` (canal 0: enabled/on/off/desired).

Notas:
- En C2 se conectará el estado “deseado” del motor a `app/outlets` en modo AUTO y se añadirá persistencia TLV de ventanas.


## B6.3c — Fix outlets registry API (bool/u32)

- Se corrige `app/outlets` para usar la API real de `EepromRegistry`:
  - Key `310` (outlet.block_valid) se lee/escribe con `getBool/setBool`.
  - Keys `311..319` (outlet.N.state) se leen/escriben con `getU32/setU32` (0/1).
- Se elimina dependencia de métodos inexistentes (`getU8/setU8`) y se restablece la compilación.


## B6.3b — Fix HAL MQTT (IMqttHal contract) + peek() safe wrapper

- Se corrige `hal_mqtt_pubsubclient.cpp` para cumplir el contrato de `IMqttHal`:
  - `disconnect()` ahora devuelve `void` (antes devolvía `MqttError`).
  - `subscribe()` ahora implementa firma `subscribe(const char* topic, uint8_t qos)`.

- Se mantiene la mejora de auditoría:
  - `TcpClientAsArduinoClient::peek()` implementado con cache de 1 byte.
  - `read()` y `read(buf,len)` consumen primero el byte cacheado si existe.

- Se corrige la const-correctness:
  - `connected() const` llama a `PubSubClient::connected()` mediante `const_cast` (ya que la librería no lo declara `const`).
```markdown

## B6.3a — Aplicación de mejoras de auditoría (robustez + coherencia)

- MQTT wrapper (PubSubClient):
  - Se implementa `peek()` en `TcpClientAsArduinoClient` con cache de 1 byte.
  - `read()` y `read(buf,len)` consumen primero el byte cacheado si existe.
  - Evita incompatibilidades futuras si la librería MQTT usa `peek()`.

- Backend legacy:
  - Se amplían buffers de topics de `64` a `128` (`_subTopic/_pubTopic`) para evitar truncado con username/apikey largos.
  - `makeTopic()` detecta truncado de `snprintf`:
    - En truncado, deja topic vacío y loggea error.
  - En `loop()`, solo se hace `subscribe()` si el topic no está vacío (evita subscribes inválidos).

- Runtime:
  - Se añade logging mínimo por etapa en `runtime::begin()` sin cambiar comportamiento:
    - fallo de `registry().begin()`
    - fallo/skip de migración legacy->registry
    - fallo de `cfg::loadOrDefault()`
    - fallo de `network().begin()` (log de `NetError`)

- Documentación NVM:
  - Se actualiza `docs/NVM_REGISTRY_KEYS_AND_EEPROM_MAP.md` para reflejar estado real:
    - Outlets `310..319` y dosing `500..585` figuran como “Migrado ya” (coherente con `eeprom_migration.cpp`).


## B6.2b — Fix build: evitar doble definición runtime (app_runtime_old)

- Se detecta fallo de link por doble definición:
  - `app_runtime.cpp` y `app_runtime_old.cpp` implementan `app::runtime::begin()` y `app::runtime::loop()`.

- Se archiva `app_runtime_old.cpp` para que no contribuya símbolos al link:
  - Se envuelve su contenido bajo `#if 0` (modo referencia/archivo histórico).
  - Mantiene trazabilidad del runtime anterior sin romper compilación en PlatformIO.


## B6.2 (unificado con B6.2a) — Scheduler FAKE/REAL con flag + HA discovery

- Scheduler backend seleccionable sin rediseño:
  - Flag: `APP_SCHEDULER_USE_RTC` (default=0)
  - Si `APP_SCHEDULER_USE_RTC=1`, scheduler intenta leer el tiempo desde un hook débil:
    - `app_scheduler_rtc_minute_of_day()` -> devuelve `0..1439`
  - Si el hook no existe o devuelve `0xFFFF`, fallback automático a `millis()` (modo FAKE).

- Flags de módulos centralizados en `platformio.ini` (entorno mega2560_port):
  - `APP_SCHEDULER_USE_RTC=0`
  - `APP_OUTLETS_USE_RELAY_HAL=0`

- HA state ampliado para validación del scheduler:
  - `ferduino/<deviceId>/state` incluye:
    - `clock_minute_of_day`, `clock_hour`, `clock_minute`, `clock_tick`, `clock_source`

- HA discovery actualizado (`homeassistant/.../config`):
  - Nuevos sensores:
    - `clock_minute_of_day` (min)
    - `clock_source` ("millis" | "rtc")
    - `clock_tick` (0/1)
  - Permite verificar switchover FAKE→REAL desde Home Assistant sin tocar código de lógica.


## B6.2a — Scheduler backend seleccionable (millis vs RTC) sin rediseño

- Se introduce flag de compilación:
  - `APP_SCHEDULER_USE_RTC` (default = 0)

- `app/scheduler` mantiene API estable y ahora soporta backend RTC sin acoplarse a HAL:
  - Si `APP_SCHEDULER_USE_RTC=1`, se intenta leer minuto del día desde un hook débil:
    - `app_scheduler_rtc_minute_of_day()` -> devuelve `0..1439`
  - Si el hook no existe o devuelve `0xFFFF`, fallback automático a `millis()`.

- Tick de scheduler:
  - `minuteTick()` se genera comparando cambios de `minuteOfDay()` (válido tanto para millis como para RTC).

- Telemetría debug:
  - `ferduino/<deviceId>/telemetry/scheduler` incluye campo:
    - `"source": "millis" | "rtc"`
  - Permite verificar el switchover sin cambiar lógica de app.

- Cuando haya hardware:
  - Activar `-D APP_SCHEDULER_USE_RTC=1`
  - Implementar `app_scheduler_rtc_minute_of_day()` en HAL/RTC sin tocar el scheduler ni los módulos consumidores.


## B6.1b — Outlets runtime (registry TLV) + integración HA (stub) + fix build strings

- Se corrige `app/config/app_config.h`:
  - `MqttConfig` pasa de `const char*` a buffers `char[64]` para permitir edición en runtime.
  - Evita errores de compilación tipo “assignment of read-only location” al hacer `strncpy()` y null-terminate.

- Se ajusta `app/config/app_config.cpp` (cambios mínimos):
  - Mantiene `clientId` alineado con `deviceId` por defecto.
  - Continúa cargando/guardando por registry TLV (sin tocar EEPROM legacy 0..1023).

- Se crea módulo `app/outlets`:
  - Archivos:
    - `include/app/outlets/app_outlets.h`
    - `src/app/outlets/app_outlets.cpp`
  - API:
    - `begin()`, `set(idx,state)`, `get(idx)`
  - Persistencia TLV:
    - Keys `311..319` (U32 0/1)
  - Se adapta a la firma real de registry:
    - `getU32(key, out)` (no devuelve valor).

- Se actualiza backend HA (`src/app/comms/comms_ha_backend.cpp`):
  - Elimina dependencia de `ha_outlet_control`.
  - `cmd/outlet/<n>` → `app::outlets::set(n-1, state)`
  - `state` publica outlets desde `app::outlets::get()`.

- Implementación STUB:
  - `APP_OUTLETS_USE_RELAY_HAL` (default=0)
  - No se accede a hardware (PCF8575) en este paso.


## B6.1a — Sensores (Temperatura) FAKE + telemetría debug

- Se crea módulo `app/sensors` con tipos y API:
  - `Temperatures { water/air, valid, ts_ms }`
  - `begin()`, `loop()`, `last()`
- Implementación FAKE (sin hardware) en `src/app/sensors/sensors.cpp`:
  - water ~25.1°C y air ~24.7°C con variación lenta determinista.
- Se añade telemetría MQTT debug:
  - Topic: `ferduino/<deviceId>/telemetry/temps` (no retained)
  - Payload: JSON con x10 y flags de validez
- Integración en `app/runtime/app_runtime.cpp`:
  - `sensors::begin/loop`
  - `publishTempsLoop(30)`


## Pre-B6 — Fixes audit Codex (consistencia y robustez)

- SMOKETEST:
  - Se definen constantes numéricas en `include/app/app_smoketests.h`.
  - Se añade `SMOKETEST_NONE`.
  - En `src/main.cpp` se elimina fallback silencioso y se fuerza `#error` si SMOKETEST no está soportado.

- Docs NVM:
  - `docs/NVM_REGISTRY_KEYS_AND_EEPROM_MAP.md`: Outlets (310..319) y Dosing (500..585) pasan a estado "Migrado ya" para reflejar `src/app/nvm/eeprom_migration.cpp`.

- Comentarios config:
  - `include/app/config/app_config.h`: comentarios del API actualizados para reflejar NVM registry TLV en vez de "EEPROM directa".


## B5.7 — Info retained + telemetría mínima (uptime)

Se añade publicación de “birth certificate” y heartbeat:

- `ferduino/<deviceId>/info` (retained)
  - fw, build, board, backend, boot/mcusr, mac, ip/gw/subnet/dns, link

- `ferduino/<deviceId>/telemetry/uptime` (no retained)
  - cada 30s publica `{"ms":<millis>}`

Integración:
- En `app::runtime::loop()`, al flanco MQTT connected:
  - `publishStatusRetained()`
  - `publishInfoRetained()`
- En cada loop: `telemetryLoop(30)`

## B5.6 — MQTT LWT (Last Will) para offline retained

Se amplía la HAL MQTT para soportar LWT (Last Will & Testament) mediante `MqttConfig`:
- `willTopic`, `willPayload`, `willQos`, `willRetained`.

Implementación en `hal_mqtt_pubsubclient.cpp`:
- Si el caller NO configura will (willTopic null/vacío), el HAL autogenera:
  - Topic: `ferduino/<clientId>/status`
  - Payload: `{"online":0}`
  - retained=true, qos=0

Objetivo:
- Si la placa pierde alimentación o se cuelga, el broker marca offline automáticamente sin intervención del firmware.


## B5.5 — Status retained + boot reason (runtime)

Se publica un estado retained al conectar MQTT (flanco desconectado->conectado) sin tocar backends.

- Topic: `ferduino/<deviceId>/status` (retained)
- Payload JSON:
  - `online` (1)
  - `backend` ("HA" / "LEGACY")
  - `boot` (power_on/external/brown_out/watchdog/jtag/unknown)
  - `mcusr` (valor bruto capturado en AVR)
  - `fw` (FERDUINO_FW_VERSION o "dev")
  - `build` (__DATE__ __TIME__)
  - `ip` (IPv4)

Implementación:
- Nuevo: `app/runtime/app_boot_reason.{h,cpp}` (captura MCUSR temprana)
- Nuevo: `app/runtime/app_status.{h,cpp}` (builder + publish retained)
- Runtime (`app/runtime/app_runtime.cpp`) detecta flanco `hal::mqtt().connected()` y publica status.


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
