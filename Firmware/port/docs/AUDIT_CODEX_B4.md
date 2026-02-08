# AUDIT CODEX B4 - ferduino-next (Firmware/port)

- Fecha: 2026-02-08
- Commit auditado: `b29ef64`
- Alcance: solo `Firmware/port` (sin cambios en `Firmware/Original`)

## Comandos usados

```powershell
git rev-parse --short HEAD
Get-Date -Format "yyyy-MM-dd"
Get-ChildItem -Path Firmware\port -Recurse | Select-Object FullName
rg --files Firmware/port/include Firmware/port/src
rg -n "legacy|homeassistant|discovery|topic|EEPROM|TLV|registry|migrat|String|malloc|new|delete|mega2560|PubSubClient|Ethernet|subscribe|publish" Firmware/port/src Firmware/port/include
rg -n "case\s+[0-9]+|publishHome_ID0|publishUnknown|topic/command|topic/response|makeTopic|snprintf\(|bridgeFromLegacyHomeJson|handleConfigCommand" Firmware/port/src/app/comms/comms_legacy_backend.cpp
rg -n "enum|KEY_|REGF_|LEGACY_|OFFSET_|migrateLegacyIfNeeded|readLegacy|write|TLV|CRC|MAGIC|VERSION" Firmware/port/include/app/nvm/eeprom_layout.h Firmware/port/include/app/nvm/eeprom_registry.h Firmware/port/src/app/nvm/eeprom_registry.cpp Firmware/port/src/app/nvm/eeprom_migration.cpp
rg -n "homeassistant|cmd/outlet|state|availability|publishDiscovery|subscribe|handleOutletCommand|setOutletState|legacy|bridge" Firmware/port/src/app/comms/comms_ha_backend.cpp Firmware/port/src/app/comms/ha/ha_discovery.cpp Firmware/port/src/app/comms/ha/ha_legacy_bridge.cpp Firmware/port/src/app/comms/ha/ha_outlet_control.cpp
rg -n "\bString\b|\bnew\b|\bdelete\b|malloc\(|free\(|realloc\(|std::vector|std::string" Firmware/port/src Firmware/port/include
rg -n "app::comms|comms\(\)\.begin|comms\(\)\.loop|hal::network\(\)\.begin" Firmware/port/src Firmware/port/include
rg -n "B5|B6|roadmap|hoja de ruta" docs Firmware/port/include Firmware/port/src
```

## A) Estructura y mapa de módulos

### Árbol relevante (`Firmware/port`)

```text
Firmware/port/
  platformio.ini
  include/
    app/
      app_smoketests.h
      comms_backend.h
      comms_mode.h
      config/
        app_config.h
        app_config_mqtt_admin.h
      nvm/
        eeprom_layout.h
        eeprom_registry.h
        eeprom_migration.h
    hal/
      hal_board.h
      hal_gpio.h
      hal_i2c.h
      hal_ioexpander.h
      hal_log.h
      hal_mqtt.h
      hal_network.h
      hal_pcf8575.h
      hal_pwm.h
      hal_relay.h
      hal_rtc.h
      hal_serial.h
      hal_storage.h
      hal_time.h
    board/
      board_config.h
      board_debug.h
      board_hw_config.h
      ferduino_config.h
      ferduino_original_config.h
    pins/
      pins.h
      pins_ferduino_mega2560.h
      pins_profiles.h
  src/
    main.cpp
    app/
      comms/
        comms.cpp
        comms_legacy_backend.cpp
        comms_ha_backend.cpp
        comms_ha_backend.h
        ha/
          ha_discovery.cpp
          ha_discovery.h
          ha_legacy_bridge.cpp
          ha_legacy_bridge.h
          ha_outlet_control.cpp
          ha_outlet_control.h
      config/
        app_config.cpp
        app_config_mqtt_admin.cpp
      nvm/
        eeprom_registry.cpp
        eeprom_migration.cpp
      smoketests/
        app_ferduino_pins_smoketest.cpp
        app_gpio_smoketest.cpp
        app_ioexpander_smoketest.cpp
        app_mqtt_smoketest.cpp
        app_network_smoketest.cpp
        app_pcf8575_smoketest.cpp
        app_pwm_leds_smoketest.cpp
        app_relay_smoketest.cpp
        app_rtc_smoketest.cpp
        app_serial_smoketest.cpp
    hal/
      impl/mega2560/
        hal_i2c.cpp
        hal_ioexpander.cpp
        hal_mqtt_pubsubclient.cpp
        hal_network_ethernet_w5x00.cpp
        hal_pcf8575.cpp
        hal_pwm.cpp
        hal_relay.cpp
        hal_rtc.cpp
        hal_serial.cpp
        hal_storage_eeprom.cpp
```

### Mapa funcional

- HAL desacoplada por interfaces:
  - Red: `include/hal/hal_network.h`
  - MQTT: `include/hal/hal_mqtt.h`
  - Storage: `include/hal/hal_storage.h`
- Implementación actual concreta (solo Mega2560):
  - `src/hal/impl/mega2560/hal_network_ethernet_w5x00.cpp`
  - `src/hal/impl/mega2560/hal_mqtt_pubsubclient.cpp`
  - `src/hal/impl/mega2560/hal_storage_eeprom.cpp`
- App/Comms:
  - Selector runtime backend: `src/app/comms/comms.cpp` -> `app::comms()`
  - Legacy backend: `src/app/comms/comms_legacy_backend.cpp` (`CommsLegacyBackend`)
  - HA backend: `src/app/comms/comms_ha_backend.cpp` (`CommsHABackend`)
- HA submódulos:
  - Discovery: `src/app/comms/ha/ha_discovery.cpp` (`publishDiscoveryMinimal`, `publishDiscoveryAll`)
  - Legacy->HA bridge: `src/app/comms/ha/ha_legacy_bridge.cpp` (`bridgeFromLegacyHomeJson`)
  - Outlets runtime: `src/app/comms/ha/ha_outlet_control.cpp` (`applyOutletCommand`, `getOutletState`)
- NVM:
  - Layout dual: `include/app/nvm/eeprom_layout.h`
  - Registry TLV: `include/app/nvm/eeprom_registry.h`, `src/app/nvm/eeprom_registry.cpp`
  - Migración legacy->registry: `src/app/nvm/eeprom_migration.cpp` (`migrateLegacyIfNeeded`)
- Smoketests:
  - Declaración: `include/app/app_smoketests.h`
  - Ejecución por `SMOKETEST` en `src/main.cpp`

## B) Portabilidad

### Riesgos de casing/includes/dependencias

- Casing `nvm/NVM`:
  - Código fuente actual usa `app/nvm/...` consistentemente.
  - Hay inconsistencia documental en `docs/07_CONTEXT.md` (`src/app/NVM/eeprom_migration.cpp`), que rompería automatización en FS case-sensitive.
- Includes dependientes de Arduino/AVR:
  - Fuertemente AVR/Arduino en implementaciones y wrappers (`Arduino.h`, `Wire.h`, `EEPROM.h`, `Ethernet.h`, `PubSubClient.h`, `RTClib.h`).
  - `include/hal/hal_gpio.h`, `include/hal/hal_time.h`, `include/hal/hal_log.h` ya incluyen `Arduino.h`; no están completamente aislados de plataforma.
- Dependencia del árbol Original:
  - `platformio.ini` (`-I../Original/Ferduino_Aquarium_Controller`) en `mega2560_port`.
  - `include/board/ferduino_original_config.h` incluye `<Configuration.h>` (aunque no parece usado en runtime normal).
- Heap/String:
  - No se detecta uso de `String`, `new/delete` ni `malloc/free` en `Firmware/port/src` y `Firmware/port/include`.
  - Se mantiene enfoque stack+buffers fijos (positivo para AVR).

### Qué está acoplado a mega2560 y qué está desacoplado

- Acoplado a Mega2560/AVR:
  - Implementaciones en `src/hal/impl/mega2560/*` con `#if defined(ARDUINO_AVR_MEGA2560) || defined(ARDUINO_ARCH_AVR)`.
  - Ethernet W5x00 (`hal_network_ethernet_w5x00.cpp`), MQTT PubSubClient (`hal_mqtt_pubsubclient.cpp`), EEPROM AVR (`hal_storage_eeprom.cpp`), serial con `Serial1..Serial3` (`hal_serial.cpp`).
  - Pinmap concreto Mega en `include/pins/pins_ferduino_mega2560.h`.
- Bien desacoplado (contrato estable):
  - Interfaces HAL: `hal_network.h`, `hal_mqtt.h`, `hal_storage.h`.
  - Capa app/comms usa interfaces, no clases Arduino directas.
  - NVM registry (`eeprom_registry.*`) usa `hal::storage()` y no llama EEPROM directa.

## C) Compatibilidad con Ferduino original

### Protocolo legacy implementado (IDs/topics)

- Topics legacy:
  - Subscribe: `Username/APIKEY/topic/command`
  - Publish: `Username/APIKEY/topic/response`
  - Construcción en `makeTopic()` de `src/app/comms/comms_legacy_backend.cpp`.
- Framing legacy:
  - Parsing de `K` terminador y tokens CSV en `CommsLegacyBackend::onMqttMessage`.
- IDs implementados:
  - `0..17` con switch explícito en `CommsLegacyBackend::onMqttMessage`.
  - Funciones: `publishHome_ID0`, `handleDs18b20_ID1`, ..., `handleFilePump_ID17`, `publishUnknown`.
- Estado real:
  - Estructura del protocolo y keys JSON sí están implementadas.
  - Casi todos los valores están en placeholders (0/"") y muchos `mode!=0` hacen ACK `{"response":"ok"}` sin aplicar hardware/config aún.

### Estado HA (discovery, topics, bridge)

- Backend HA operativo a nivel MQTT/topics:
  - State: `ferduino/<deviceId>/state`
  - Availability: `ferduino/<deviceId>/availability`
  - Commands outlets: `ferduino/<deviceId>/cmd/outlet/<1..9>`
  - Config admin: `ferduino/<deviceId>/cmd/config`
- Discovery:
  - Publicación retained de sensores y switches en `homeassistant/.../config` desde `ha_discovery.cpp`.
  - `publishDiscoveryMinimal()` y `publishDiscoveryAll()` son equivalentes (mismo conjunto actual).
- Bridge legacy->HA:
  - `bridgeFromLegacyHomeJson()` transforma JSON ID0 legacy a state HA (snake_case).
  - Se llama desde `publishHome_ID0()` en backend legacy.
- Outlets:
  - `applyOutletCommand()` y `getOutletState()` funcionan en RAM.
  - No está cableado a `hal::relay()` (hay TODO explícito).

### EEPROM legacy vs registry TLV

- Contrato de layout dual existe y es claro:
  - Legacy RO: `0..1023`
  - Registry RW: `1024..4095`
  - Definido en `include/app/nvm/eeprom_layout.h`.
- Registry TLV implementado:
  - Entry: `[key:u16][type:u8][len:u8][value:len]`
  - CRC32 de payload completo, flags (`REGF_MIGRATED`), singleton `registry()`.
- Migración legacy implementada (subset):
  - `migrateLegacyIfNeeded()` migra solo temp water set/off/alarm:
    - offsets legacy `482`, `484`, `485`
    - keys TLV `100`, `101`, `102`
  - Marca `REGF_MIGRATED`.
- Gap crítico:
  - `app_config.cpp` persiste config en EEPROM offset `0` (`EEPROM_ADDR=0`), fuera del registry y dentro de zona legacy reservada.

## D) Riesgos y deuda técnica (priorizado)

### P0

1. Colisión de persistencia con zona legacy reservada
- Archivo: `Firmware/port/src/app/config/app_config.cpp`
- Funciones: `loadOrDefault()`, `save()`, `factoryReset()`
- Evidencia: usa `EEPROM_ADDR = 0` y `EEPROM.get/put` directo.
- Riesgo: sobrescribe offsets legacy; rompe el contrato RO de `eeprom_layout.h` y compatibilidad con firmware original.
- Recomendación: mover `app::cfg` a `app::nvm::registry()` (claves TLV), mantener `0..1023` estrictamente solo lectura.

2. Runtime principal no arranca stack de comms/network
- Archivo: `Firmware/port/src/main.cpp`
- Funciones: `setup()`, `loop()`
- Evidencia: solo ejecuta smoketests; no hay `app::comms().begin()` ni `app::comms().loop()`.
- Riesgo: protocolo legacy/HA no está activo en ejecución normal de firmware port.
- Recomendación: introducir entrypoint de runtime (sin smoketest) con inicialización ordenada: cfg/nvm -> network -> comms.

### P1

1. Control de outlets HA no actúa sobre relés reales
- Archivo: `Firmware/port/src/app/comms/ha/ha_outlet_control.cpp`
- Función: `applyOutletCommand()`
- Evidencia: solo estado RAM; TODO hacia `hal::relay`.
- Riesgo: HA aparenta control pero no conmuta hardware.
- Recomendación: mapear outlet 1..9 a `hal::Relay` y persistir/replicar estado real.

2. Implementación legacy funcional pero mayormente placeholder
- Archivo: `Firmware/port/src/app/comms/comms_legacy_backend.cpp`
- Funciones: `publishHome_ID0()`, `handle*_ID*()`
- Evidencia: casi todos los campos son constantes o ACK sin aplicar cambios.
- Riesgo: compatibilidad de forma (JSON/IDs) sin compatibilidad de comportamiento.
- Recomendación: portar por ID, empezando por los de mayor impacto (ID0, 2, 9, 13-16, outlets).

3. Tópicos legacy con buffers pequeños
- Archivo: `Firmware/port/src/app/comms/comms_legacy_backend.cpp`
- Campos: `_subTopic[64]`, `_pubTopic[64]`
- Evidencia: topic depende de `username/apikey`; con credenciales largas puede truncar silenciosamente.
- Riesgo: suscripción/publicación en topic incorrecto.
- Recomendación: validar longitud de credenciales o ampliar buffers y comprobar retorno de `snprintf`.

### P2

1. Inconsistencia de casing en documentación
- Archivo: `docs/07_CONTEXT.md`
- Evidencia: referencia `src/app/NVM/eeprom_migration.cpp` vs ruta real `src/app/nvm/eeprom_migration.cpp`.
- Riesgo: scripts/documentación que fallen en Linux/macOS case-sensitive.
- Recomendación: normalizar referencias a `nvm` minúsculas.

2. Dependencia de include path de Original en entorno `mega2560_port`
- Archivo: `Firmware/port/platformio.ini`
- Evidencia: `-I../Original/Ferduino_Aquarium_Controller` en entorno de port.
- Riesgo: acoplamiento innecesario y menor portabilidad del build.
- Recomendación: eliminar dependencia cuando ningún módulo del port requiera headers del original.

## E) Próximos pasos (B5/B6) con commits pequeños

### B5 - "usar registry en runtime"

1. Commit B5.1 - Definir keys TLV para config runtime
- Añadir catálogo de keys (backend, mqtt host/port/deviceId, net mac/dhcp/ip/gw/subnet/dns).
- Archivos: `include/app/nvm/eeprom_registry.h` (o nuevo header de keys), `src/app/nvm/eeprom_migration.cpp`.

2. Commit B5.2 - `app::cfg` backend dual con prioridad registry
- `app_config.cpp`: leer/escribir desde registry; fallback a defaults solo si faltan keys.
- Mantener compat temporal de lectura legacy sin escribir zona 0..1023.

3. Commit B5.3 - Migración extendida legacy->registry
- Ampliar de temp (482..485) a bloques críticos del mapa legacy (`docs/11_eeprom_legacy_map.md`): pH/ORP/salinidad/outlets y red si aplica.
- `migrateLegacyIfNeeded()` idempotente.

4. Commit B5.4 - Main runtime no-smoketest
- En `main.cpp`, modo productivo opcional por flag (`SMOKETEST_NONE`) que ejecute `app::comms().begin/loop`.

### B6 - "porting incremental de comportamiento"

1. Commit B6.1 - Cablear HA outlets a relés HAL
- `ha_outlet_control.cpp`: map outlet->`hal::Relay` y lectura de estado real.

2. Commit B6.2 - ID0 con datos reales (sensores/estado)
- `comms_legacy_backend.cpp::publishHome_ID0()` alimentado por HAL real (RTC, relay/outlets, sensores disponibles).

3. Commit B6.3 - IDs de escritura críticos con persistencia registry
- Prioridad: ID2 (RTC), ID9, ID13-16, ID17.
- Aplicar cambios + persistir en registry; respuesta legacy idéntica.

4. Commit B6.4 - Pruebas de regresión protocolo
- Smoketest/script de roundtrip MQTT legacy/HA con casos de IDs y validación de keys JSON/topics.

## Conclusión ejecutiva

- Arquitectura base HAL + comms + NVM está bien encaminada y ya separa interfaces de implementaciones.
- El mayor bloqueo actual para compatibilidad real es de persistencia: `app::cfg` escribe en EEPROM offset 0 y contradice el layout dual B4.
- Legacy y HA ya tienen esqueleto de protocolo/topics/discovery útil para porting incremental, pero el comportamiento funcional aún está mayormente en placeholder.
