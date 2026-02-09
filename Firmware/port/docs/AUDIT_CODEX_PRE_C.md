# AUDIT_CODEX_PRE_C

- Fecha: 2026-02-09 22:51:30 +00:00
- Commit auditado: `45fe6cd`
- Alcance: `Firmware/port` (implementación) + `Firmware/Original/Ferduino_Aquarium_Controller` (referencia legacy)

## A) Estado del Build

**Estado:** `FAIL (no verificable localmente)`

Comandos ejecutados:
- `platformio run -d Firmware/port -e mega2560_port` -> no disponible
- `pio run -d Firmware/port -e mega2560_port` -> no disponible
- `python -m platformio run -d Firmware/port -e mega2560_port` -> módulo no instalado

Observaciones:
- Hay artefactos previos en `Firmware/port/.pio/build/mega2560_port` (`firmware.elf`, `firmware.hex`), pero no sirven como evidencia de compilación del estado actual.
- Con revisión estática hay incompatibilidades que **probablemente** rompen link/compilación cuando el runtime B6.x se activa.

Warnings/risks relevantes detectados (estáticos):
1. **CRITICAL** API scheduler inconsistente entre header e implementación.
- Header declara `now()/minutesSinceBoot()/minuteTick()/timeSource()` en `Firmware/port/include/app/scheduler/app_scheduler.h:25`.
- Implementación define `hour()/minute()/usingRtc()/tickMs()` en `Firmware/port/src/app/scheduler/app_scheduler.cpp:75`.
- Consumidores llaman API del header (`Firmware/port/src/app/comms/comms_ha_backend.cpp:173`, `Firmware/port/src/app/runtime/app_telemetry.cpp:45`).

2. **CRITICAL** `APP_OUTLETS_USE_RELAY_HAL=1` parece romper compilación por API HAL incorrecta.
- Se usa `hal::relay().set(..., hal::RelayState::On/Off)` en `Firmware/port/src/app/outlets/app_outlets.cpp:59`.
- HAL real expone `relayInit()/relaySet(Relay,bool)` en `Firmware/port/include/hal/hal_relay.h:41`.

3. **WARNING** `main` no ejecuta runtime B6.x en `SMOKETEST_NONE`.
- `Firmware/port/src/main.cpp:37` deja comentario y no llama `app::runtime::loop()`.
- Esto oculta fallos reales de integración B6.x en ejecución normal.

4. **WARNING** header duplicado de outlets.
- `Firmware/port/include/app/outlets/app_outlets.h`
- `Firmware/port/src/app/outlets/app_outlets.h`
- Duplicado con mismo contenido: riesgo de deriva/ODR conceptual y resolución de include ambigua.

5. **WARNING** `mega2560_port` sigue incluyendo cabeceras del Original.
- `-I../Original/Ferduino_Aquarium_Controller` en `Firmware/port/platformio.ini:18`.
- Riesgo de acoplamiento accidental y includes fantasma.

## B) Flags FAKE/REAL por módulo

| Módulo | Flag | Backend FAKE | Backend REAL | Estado | Acción |
|---|---|---|---|---|---|
| Scheduler | `APP_SCHEDULER_USE_RTC` | `millis()` | hook RTC (`app_scheduler_rtc_minute_of_day`) | Parcial | Corregir API header/cpp y mantener fallback `0xFFFF`.
| Outlets | `APP_OUTLETS_USE_RELAY_HAL` | RAM + registry TLV (`310`,`311..319`) | HAL relés | **Roto con flag=1** | Alinear llamadas a `hal::relaySet(...)` + mapping enum.
| Sensors | `APP_SENSORS_USE_HW` | valores FAKE | stub HW (sin driver aún) | Compila por diseño | Mantener interfaz y añadir backend HW incremental en C.
| Comms | *(sin `APP_*`)* (`FERDUINO_COMMS_MODE` bootstrap + `backendMode` runtime) | Legacy backend | HA backend | Funcional pero no estándar | Mantener por compatibilidad; documentar como selector de default inicial.
| RTC | indirecto por scheduler | N/A | HAL RTC + hook | Parcial | Implementar hook fuerte en archivo dedicado (sin tocar scheduler).
| Relay HAL | indirecto por outlets | N/A | `hal_relay` | Implementado | Corregir capa app que lo consume con API actual.
| Network HAL | *(sin flag)* | N/A | Ethernet HAL | Implementado | Sin cambios, opcional agregar `APP_NETWORK_USE_ETH_HAL` si habrá backends alternos.

Coherencia de uso con `app_build_flags.h`:
- Correcto en `scheduler/outlets/sensors` (`Firmware/port/src/app/scheduler/app_scheduler.cpp:5`, `Firmware/port/src/app/outlets/app_outlets.cpp:4`, `Firmware/port/src/app/sensors/sensors.cpp:5`).
- Faltan flags por módulo para `comms/network/rtc` si se quiere uniformidad absoluta de política.

## C) Estructura de archivos (foco `src/app/comms`)

### Hallazgos

Headers públicos actualmente en `include/` (correcto):
- `Firmware/port/include/app/comms_backend.h`
- `Firmware/port/include/app/comms_mode.h`
- `Firmware/port/include/app/runtime/*`, `Firmware/port/include/app/scheduler/*`, `Firmware/port/include/app/sensors/*`, etc.

Headers ubicados en `src/` (privados de implementación):
- `Firmware/port/src/app/comms/comms_ha_backend.h`
- `Firmware/port/src/app/comms/ha/ha_discovery.h`
- `Firmware/port/src/app/comms/ha/ha_legacy_bridge.h`
- `Firmware/port/src/app/comms/ha/ha_outlet_control.h`
- `Firmware/port/src/app/outlets/app_outlets.h` (duplicado no deseado)

Includes con rutas `app/comms/...` desde `.cpp`:
- `Firmware/port/src/app/comms/comms_ha_backend.cpp:1`
- `Firmware/port/src/app/comms/comms_legacy_backend.cpp:3`
- `Firmware/port/src/app/comms/ha/ha_discovery.cpp:1`

No se detectó include explícito con prefijo `src/...`, pero sí dependencia de headers privados bajo árbol `src/app/comms`.

### Clasificación público/privado recomendada

Público (API estable):
- `include/app/comms_backend.h`
- `include/app/comms_mode.h`
- `include/app/outlets/app_outlets.h` (único)

Privado (quedarse en `src`):
- `src/app/comms/comms_ha_backend.h`
- `src/app/comms/ha/ha_discovery.h`
- `src/app/comms/ha/ha_legacy_bridge.h`
- `src/app/comms/ha/ha_outlet_control.h`

### Propuesta minimalista (Opción A, sin mover)

1. Documentar convención en `docs`: `include/app/*` público, `src/app/**.h` privado.
2. Eliminar duplicado `src/app/outlets/app_outlets.h` y dejar solo `include/app/outlets/app_outlets.h`.
3. Mantener headers privados en `src/app/comms`, pero evitar que otros módulos fuera de `comms` dependan de ellos.

Riesgo Opción A: **bajo**.

### Alternativa (Opción B, con mover)

Mover a `include/app/comms/ha/` solo si se decide que son API reutilizable externa:
- `ha_discovery.h`, `ha_legacy_bridge.h`.

Cambios requeridos:
- actualizar includes en `comms_ha_backend.cpp`, `comms_legacy_backend.cpp`, `ha_discovery.cpp`, `ha_legacy_bridge.cpp`.
- revisar LDF y colisiones de rutas.

Riesgo Opción B: **medio** (cambio de visibilidad de API + potencial deuda de compatibilidad futura).

## D) Mapa legacy -> port (preparación Fase C)

| Bloque legacy | Fuente original | Estado en port | Qué falta | Primer entregable C recomendado |
|---|---|---|---|---|
| Timers/scheduler | `.../src/Modules/Temporizadores.h:2`, `.../Ferduino_Aquarium_Controller.h:481` | Base de reloj B6 (`app_scheduler`) | Motor de eventos ON/OFF por canal y ventana cruzando medianoche | C1: `EventScheduler` sin hardware, evaluación por `minuteOfDay`.
| Outlets/reles | `.../Funcoes_EEPROM.h:273`, `.../Loop.h:117`, `.../Ferduino_Aquarium_Controller.h:96` | Estado runtime + TLV (`310`,`311..319`) + comandos HA | Modo AUTO por reglas/scheduler y mapping completo de salidas | C2: auto-control outlets por eventos + estado manual/auto.
| Dosing (dosadora) | `.../Modules/Dosadoras.h`, `.../Funcoes_EEPROM.h:131` | Migración NVM (`500..585`) ya existe | Motor de ejecución, ventanas, cantidades, reparto y anti-solape | C3: dosificación por ventana mínima (1 canal) + telemetría de ejecución.
| Iluminación PWM | `.../Modules/Leds.h`, `.../Modules/Clima.h` | HAL PWM y smoketests | Curvas/tabla horaria, rampas amanecer/anochecer, límites térmicos | C4: setpoint horario básico por canal (sin clima/rayos al inicio).
| Sensores (Dallas) | `.../Include/Library.h:14`, `.../Ferduino_Aquarium_Controller.h:31` | Módulo FAKE + telemetry `telemetry/temps` | Backend HW real DS18B20 + validación/timeout | C2/C3 paralelo: habilitar `APP_SENSORS_USE_HW=1` sin romper fallback.
| MQTT legacy / HA | `.../Modules/Webserver.h`, `.../Modules/Funcoes_ESP8266.h` | Backends Legacy + HA, discovery/state | Consolidar modelo interno y reducir placeholders | C5: publicar estado funcional real (no placeholder) desde un modelo único.

## E) Plan Fase C por commits pequeños (bajo riesgo)

1. **Fix pre-C: alinear API scheduler header/cpp**
- Archivos: `include/app/scheduler/app_scheduler.h`, `src/app/scheduler/app_scheduler.cpp`, `src/app/comms/comms_ha_backend.cpp`, `src/app/runtime/app_telemetry.cpp`
- Riesgo: medio
- Verificación: `platformio run -d Firmware/port -e mega2560_port` sin undefined refs.

2. **Fix pre-C: outlets HAL compile-safe con flag=1**
- Archivos: `src/app/outlets/app_outlets.cpp`
- Riesgo: medio
- Verificación: build con `-D APP_OUTLETS_USE_RELAY_HAL=1` compila.

3. **Fix pre-C: conectar runtime en `SMOKETEST_NONE`**
- Archivos: `src/main.cpp`
- Riesgo: medio
- Verificación: arranca runtime y publica topics base en entorno MQTT.

4. **Higiene: eliminar header duplicado outlets en `src`**
- Archivos: `src/app/outlets/app_outlets.h` (remover), includes asociados
- Riesgo: bajo
- Verificación: build limpia + no includes rotos.

5. **C1.1 Núcleo scheduler de eventos (sin HAL)**
- Archivos: `include/app/scheduler/*`, `src/app/scheduler/*` (nuevo `app_event_scheduler.*`)
- Riesgo: bajo
- Verificación: pruebas de tabla horaria (incluyendo cruce medianoche) por asserts/smoke.

6. **C1.2 Persistencia de eventos (TLV)**
- Archivos: `include/app/config/app_config_keys.h`, `src/app/nvm/eeprom_registry.cpp` (uso), `src/app/config/*`
- Riesgo: medio
- Verificación: set/get y reboot conservan eventos.

7. **C1.3 Telemetría scheduler funcional**
- Archivos: `src/app/runtime/app_telemetry.cpp`
- Riesgo: bajo
- Verificación: topic scheduler refleja próximo evento y estado del motor.

8. **C2.1 Integrar scheduler -> outlets AUTO**
- Archivos: `src/app/outlets/app_outlets.cpp`, `src/app/runtime/app_runtime.cpp`
- Riesgo: medio
- Verificación: con reloj simulado cambia outlet en minutos esperados.

9. **C2.2 Modelo manual/auto por outlet**
- Archivos: `include/app/outlets/app_outlets.h`, `src/app/outlets/app_outlets.cpp`, keys TLV nuevas
- Riesgo: medio
- Verificación: comandos manuales no rompen reglas automáticas.

10. **C2.3 HA state/discovery para modo outlet**
- Archivos: `src/app/comms/comms_ha_backend.cpp`, `src/app/comms/ha/ha_discovery.cpp`
- Riesgo: bajo
- Verificación: entidades HA muestran modo + estado real.

11. **C3.1 Motor dosing mínimo (1 canal) sobre keys 500..585**
- Archivos: nuevo `src/app/dosing/*`, integración en `app_runtime.cpp`
- Riesgo: medio
- Verificación: simulación de ventana y contador de dosis en telemetría.

12. **C3.2 Escalar dosing a 6 canales + protecciones**
- Archivos: `src/app/dosing/*`, `include/app/dosing/*`
- Riesgo: medio
- Verificación: no solape, límites por canal, persistencia estable.

13. **C4.1 PWM funcional básico por setpoint horario**
- Archivos: nuevo `src/app/lighting/*`, `hal_pwm` consumo
- Riesgo: medio
- Verificación: smoke PWM + telemetría por canal.

14. **C5.1 Config dinámica C por MQTT admin (scheduler/outlets/dosing)**
- Archivos: `src/app/config/app_config_mqtt_admin.cpp`, `include/app/config/app_config_keys.h`
- Riesgo: medio
- Verificación: set/get runtime + persistencia + reboot.

15. **C5.2 Consolidación de estado publicado (single source of truth)**
- Archivos: `src/app/runtime/*`, `src/app/comms/*`
- Riesgo: medio
- Verificación: HA y legacy leen el mismo estado interno sin divergencia.

## Conclusión de cierre B6.3

Estado recomendado: **B6.3 no debería cerrarse todavía** hasta resolver 3 bloqueantes pre-C:
1. API scheduler coherente (`header <-> cpp`).
2. Compatibilidad compilable de `APP_OUTLETS_USE_RELAY_HAL=1`.
3. Conectar `main` al runtime B6 en `SMOKETEST_NONE`.

Con esos tres ajustes, la Fase C puede arrancar con commits pequeños y trazables sin rediseño.

## Comandos usados en esta auditoría

- `git -C Firmware/port rev-parse --short HEAD`
- `Get-Date -Format "yyyy-MM-dd HH:mm:ss K"`
- `Get-Content Firmware/port/platformio.ini`
- `Get-Content Firmware/port/include/app/app_build_flags.h`
- `platformio run -d Firmware/port -e mega2560_port`
- `pio run -d Firmware/port -e mega2560_port`
- `python -m platformio run -d Firmware/port -e mega2560_port`
- `rg -n "..." Firmware/port/include Firmware/port/src`
- `rg -n "..." Firmware/Original/Ferduino_Aquarium_Controller/...`
