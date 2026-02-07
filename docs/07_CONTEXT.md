# Ferduino-next – Firmware PORT – Contexto del Proyecto

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
