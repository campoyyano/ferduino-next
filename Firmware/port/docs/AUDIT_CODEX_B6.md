# AUDIT_CODEX_PRE_B6

- Fecha: 2026-02-08 19:06:09 +00:00
- Commit auditado: `55f0fb3`
- Alcance: `Firmware/port` (solo lectura en `Firmware/Original` para contraste)

## Resumen ejecutivo
- El flujo runtime base B5.x está en orden lógico (`registry begin -> migrate -> cfg load -> network begin -> comms begin -> loops`) y está implementado en `Firmware/port/src/app/runtime/app_runtime.cpp:38`.
- Hay riesgo de configuración silenciosa: un `SMOKETEST` inválido cae al runtime normal en vez de fallar compilación (`Firmware/port/src/main.cpp:49`).
- LWT, status retained e info retained están implementados, pero existe riesgo de divergencia futura entre `clientId` y `deviceId` para topics/status (`Firmware/port/src/hal/impl/mega2560/hal_mqtt_pubsubclient.cpp:105`, `Firmware/port/src/app/runtime/app_status.cpp:32`).
- Persisten stubs explícitos previos a B6: `peek()=-1` en wrapper MQTT y control HA de salidas en RAM (`Firmware/port/src/hal/impl/mega2560/hal_mqtt_pubsubclient.cpp:49`, `Firmware/port/src/app/comms/ha/ha_outlet_control.cpp:16`).
- En backend legacy hay buffers de topic ajustados (`64`) y múltiples `snprintf` sin verificación de truncado; no rompe hoy, pero es deuda de robustez (`Firmware/port/src/app/comms/comms_legacy_backend.cpp:590`).
- El documento de mapa NVM quedó desactualizado respecto a la migración real de outlets y dosing (`Firmware/port/docs/NVM_REGISTRY_KEYS_AND_EEPROM_MAP.md:104`).
- `platformio.ini` del entorno port sigue incluyendo carpeta del Original en include path (`Firmware/port/platformio.ini:18`), lo que aumenta acoplamiento accidental.
- No se pudo validar compilación/link local porque PlatformIO no está instalado en este entorno.

## Hallazgos

### CRITICAL
1. Validación de build no ejecutable en entorno actual
- Archivos: N/A (tooling)
- Evidencia: `pio`, `platformio` y `python -m platformio` no disponibles.
- Problema: no hay confirmación empírica de compilación/link tras cierre B5.x.
- Recomendación mínima: ejecutar en CI o host con PlatformIO:
  - `platformio run -d Firmware/port -e mega2560_port`
  - y fijar este build como gate de PR antes de B6.

2. Fallback peligroso de `SMOKETEST`
- Archivo: `Firmware/port/src/main.cpp:49`
- Problema: cualquier valor inválido de `SMOKETEST` entra a `app::runtime::loop()` en vez de fallar en compilación.
- Recomendación mínima: reemplazar el `#else` por `#error "SMOKETEST value not supported"`.

### WARNING
1. Stub de `peek()` en wrapper MQTT
- Archivo: `Firmware/port/src/hal/impl/mega2560/hal_mqtt_pubsubclient.cpp:49`
- Problema: devuelve `-1` siempre; puede romper parsing si una versión/lib usa `peek()`.
- Recomendación mínima: implementar `return _c.peek();` en `ITcpClient` (o buffer 1 byte) y propagar en wrapper.

2. Riesgo de truncación de topics legacy
- Archivo: `Firmware/port/src/app/comms/comms_legacy_backend.cpp:590`
- Problema: `_subTopic[64]`/`_pubTopic[64]` pueden truncar con username/apikey largos; `snprintf` no se verifica.
- Recomendación mínima: aumentar a `128` y comprobar retorno de `snprintf` (`>= outLen` => error/log y no suscribir).

3. Múltiples `snprintf` sin control de retorno
- Archivos: `Firmware/port/src/app/comms/comms_legacy_backend.cpp` (p.ej. `:243`, `:304`, `:497`), `Firmware/port/src/app/runtime/app_info.cpp:42`, `Firmware/port/src/app/runtime/app_status.cpp:32`
- Problema: truncado silencioso de JSON/topic.
- Recomendación mínima: helper local `safe_snprintf` con chequeo de truncación y salida temprana.

4. Acoplamiento con Original en entorno port
- Archivo: `Firmware/port/platformio.ini:18`
- Problema: `-I../Original/Ferduino_Aquarium_Controller` permite dependencias cruzadas no intencionadas.
- Recomendación mínima: remover include del env `mega2560_port` cuando ya no sea necesario para compilar B5/B6.

5. Comentarios de API config desactualizados
- Archivo: `Firmware/port/include/app/config/app_config.h:45`
- Problema: comentario indica EEPROM, implementación real usa registry TLV.
- Recomendación mínima: actualizar comentario para evitar decisiones equivocadas en B6.

6. Inconsistencia docs vs código en migración NVM
- Archivos: `Firmware/port/docs/NVM_REGISTRY_KEYS_AND_EEPROM_MAP.md:104`, `Firmware/port/src/app/nvm/eeprom_migration.cpp:149`
- Problema: docs marcan outlets/dosing como pendientes, pero código ya migra `310..319` y `500..585`.
- Recomendación mínima: corregir estados a “Migrado ya” y añadir productor/consumidor actuales.

7. Riesgo de divergencia topic LWT/status por `clientId` vs `deviceId`
- Archivos: `Firmware/port/src/hal/impl/mega2560/hal_mqtt_pubsubclient.cpp:105`, `Firmware/port/src/app/runtime/app_status.cpp:32`
- Problema: hoy coinciden por config, pero si divergen habrá dos topics de status.
- Recomendación mínima: definir política única y reutilizar un mismo campo para ambos paths (o documentar separación explícita).

8. Runtime ignora errores de inicialización
- Archivo: `Firmware/port/src/app/runtime/app_runtime.cpp:44`
- Problema: varios retornos se descartan con `(void)`; difícil diagnosticar fallos de NVM/red.
- Recomendación mínima: log de error por etapa y flag de salud runtime.

### NICE-TO-HAVE
1. Headers HAL con `Arduino.h` en interfaz pública
- Archivos: `Firmware/port/include/hal/hal_gpio.h:2`, `Firmware/port/include/hal/hal_log.h:2`, `Firmware/port/include/hal/hal_time.h:2`
- Problema: reduce portabilidad a no-Arduino host tests.
- Recomendación mínima: mover dependencias Arduino a `.cpp` o tipos puente (`uint8_t`, `uint32_t`) en headers.

2. Placeholder HA aún activo
- Archivo: `Firmware/port/src/app/comms/comms_ha_backend.cpp:91`
- Problema: publica estado placeholder cada 5s; útil para smoke, pero confunde semántica productiva.
- Recomendación mínima: guardarlo detrás de flag de desarrollo o marcar explícitamente payload placeholder.

3. TODO pendiente en control HA de relés
- Archivo: `Firmware/port/src/app/comms/ha/ha_outlet_control.cpp:16`
- Problema: estado en RAM sin persistencia/acción real.
- Recomendación mínima: conectar al adapter de relés en B6.2 manteniendo fallback simulado para test sin placa.

## Revisión de flujo runtime
- Secuencia observada (OK):
  1. `registry().begin()` (`Firmware/port/src/app/runtime/app_runtime.cpp:44`)
  2. `migrateLegacyIfNeeded()` (`Firmware/port/src/app/runtime/app_runtime.cpp:45`)
  3. `cfg::loadOrDefault()` (`Firmware/port/src/app/runtime/app_runtime.cpp:47`)
  4. `network().begin()` (`Firmware/port/src/app/runtime/app_runtime.cpp:51`)
  5. `comms().begin()` (`Firmware/port/src/app/runtime/app_runtime.cpp:53`)
  6. loop: network/comms + flanco MQTT + telemetry (`Firmware/port/src/app/runtime/app_runtime.cpp:61`)
- Gap: no hay manejo explícito de error por etapa (solo best-effort).

## Revisión LWT + status/info retained
- LWT default automático: `ferduino/<clientId>/status`, retained, payload `{"online":0}` (`Firmware/port/src/hal/impl/mega2560/hal_mqtt_pubsubclient.cpp:105`).
- Status retained en runtime: `ferduino/<deviceId>/status`, payload online+uptime+boot (`Firmware/port/src/app/runtime/app_status.cpp:32`).
- Info retained en runtime: `ferduino/<deviceId>/info` (`Firmware/port/src/app/runtime/app_info.cpp:42`).
- Conclusión: soporte existe y funciona conceptualmente; falta blindar identidad única para evitar dualidad de topics.

## Consistencia registry/docs
- Código define y usa claves de config: `200,201,204,205,210..215` (`Firmware/port/include/app/config/app_config_keys.h:12`).
- Migración implementa temps `100..102`, outlets `310..319`, dosing `500..585` (`Firmware/port/src/app/nvm/eeprom_migration.cpp:17`).
- `NVM_REGISTRY_KEYS_AND_EEPROM_MAP.md` aún marca parte de esos bloques como pendientes.
- Corrección exacta propuesta:
  - En `Firmware/port/docs/NVM_REGISTRY_KEYS_AND_EEPROM_MAP.md`, filas offsets `840`, `841..849`, `706`, `545`, `557..562`, `583..588`, `589..625`, `631..636`, `637..642`, `643..648`, `649..654`: cambiar estado a `Migrado ya`.
  - Añadir nota: “migración de dosing day-mask usa codificación bitmask en `KEY_DOSING_CH1_DAYS_MASK..KEY_DOSING_CH6_DAYS_MASK`”.

## Things left behind
- `peek()` no implementado (`hal_mqtt_pubsubclient.cpp:49`).
- TODO explícito relay mapping HA (`ha_outlet_control.cpp:16`).
- Comentarios API config arrastran referencia EEPROM (`app_config.h:45`).
- `SMOKETEST` inválido no falla en compile (`main.cpp:49`).

## Ready for B6 checklist
- Build reproducible (mega2560_port) verificado en CI/host con PlatformIO: **NO**
- Flujo runtime base completo: **SI**
- LWT + status/info retained funcional: **SI (con riesgo de identidad topic)**
- NVM migración idempotente (`REGF_MIGRATED`): **SI**
- Documentación NVM alineada con código: **NO**
- Stubs críticos cerrados para B6 inicial: **NO**

## Acciones recomendadas (orden)
1. Forzar fallo de compilación ante `SMOKETEST` inválido (`main.cpp`).
2. Ejecutar build real `mega2560_port` en entorno con PlatformIO y registrar resultado.
3. Corregir docs NVM para reflejar migración real de outlets/dosing.
4. Subir robustez MQTT/comms: ampliar buffers topic y chequear truncación `snprintf`.
5. Resolver `peek()` para compatibilidad futura de PubSubClient.
6. Añadir logging mínimo de errores en `runtime::begin()` sin cambiar comportamiento externo.
7. Definir política única `clientId/deviceId` para topics de status/LWT.

## 3 refactors (alto impacto, bajo riesgo, 1-2 commits)

### Refactor 1: Guardrails de configuración de build
- Objetivo: eliminar arranque accidental con `SMOKETEST` inválido y clarificar variantes.
- Archivos: `Firmware/port/src/main.cpp`, `Firmware/port/platformio.ini` (solo comentarios/defines coherentes).
- Riesgo: bajo (solo preprocesador/config).
- Rollback: revertir commit único si bloquea builds existentes.

### Refactor 2: Helper de formato seguro para topics/json pequeños
- Objetivo: evitar truncación silenciosa y centralizar chequeos de `snprintf`.
- Archivos: `Firmware/port/src/app/comms/comms_legacy_backend.cpp`, opcional helper local en `Firmware/port/src/app/runtime/*`.
- Riesgo: bajo-medio (toca rutas de publicación, sin cambiar payload esperado si no hay truncado).
- Rollback: revertir commit; comportamiento previo intacto.

### Refactor 3: Contrato de identidad MQTT (topic key)
- Objetivo: unificar fuente para topic base (`clientId` vs `deviceId`) sin cambiar payloads.
- Archivos: `Firmware/port/src/app/runtime/app_status.cpp`, `Firmware/port/src/app/runtime/app_info.cpp`, `Firmware/port/src/hal/impl/mega2560/hal_mqtt_pubsubclient.cpp`, documentación en `Firmware/port/docs/*`.
- Riesgo: bajo si se mantiene valor actual por defecto.
- Rollback: revertir commit y volver a construcción actual de topic.

## Comandos usados durante el análisis
- `git status --short`
- `git -C Firmware/port rev-parse --short HEAD`
- `Get-Date -Format "yyyy-MM-dd HH:mm:ss K"`
- `platformio run -d Firmware/port -e mega2560_port` (falló: no instalado)
- `pio run -d Firmware/port -e mega2560_port` (falló: no instalado)
- `python -m platformio run -d Firmware/port -e mega2560_port` (falló: no instalado)
- `rg -n "..." Firmware/port/...` (múltiples búsquedas de flujo runtime, keys, TODOs, topics, flags)
- `Get-Content <archivo> | %{ ... }` (inspección con numeración de líneas)
