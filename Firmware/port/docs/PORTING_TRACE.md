# Porting Trace — Ferduino (Original -> port/HAL)

- Objetivo: trazar **qué funcionalidades** del Ferduino original han sido portadas al árbol `Firmware/port`, con:
  - ubicación (archivo/módulo),
  - estado (stub / parity / real HW),
  - flags de compilación,
  - telemetría/MQTT,
  - y notas de paridad vs original.

> Nota: este documento es **acumulativo**. Añadir entradas; no reescribir secciones antiguas.

## Convenciones

- **Estado**
  - `STUB`: compila y ejecuta, pero no hace IO real.
  - `PARITY`: lógica alineada con el original (o documentada), aún sin HW.
  - `REAL`: usa HAL + HW real.
- **Flags**
  - `-D...` via `platformio.ini -> build_flags` para habilitar/deshabilitar módulos.
- **MQTT**
  - `retained`: `true/false`
  - `topic`: forma del topic, con `<deviceId>` si aplica.

## Entradas

### C2.2 — Alertas por salida forzada (manual override)

- **Funcionalidad**: si cualquier salida está en modo manual (`auto=false`), publicar aviso recurrente hasta volver a auto.
- **Estado**: `STUB/PARITY` (no depende de HW; depende de `outlets::isAuto()`).
- **Módulo port**:
  - `include/app/alerts/forced_outlet_alert.h`
  - `src/app/alerts/forced_outlet_alert.cpp`
  - integrado en `src/app/runtime/app_runtime.cpp` (`begin/loop`)
- **NVM registry keys**:
  - `360` `alerts.forced_outlets.reminder_minofday` (`U32`) — minuto del día del recordatorio (default 09:00)
  - `361` `alerts.forced_outlets.enabled` (`bool`) — enable/disable
- **MQTT**
  - topic: `ferduino/<deviceId>/alert/forced_outlets` (no retained)
  - payload: `{"active":true|false,"mask":<u16>,"reason":"change|daily|disabled","minOfDay":<u16>}`
- **Comandos HA backend**
  - `ferduino/<deviceId>/cmd/forced_alert_time` payload `"HH:MM"` o `{"time":"HH:MM"}` o `"get"`
  - `ferduino/<deviceId>/cfg/forced_alert_time` retained `"HH:MM"`
  - `ferduino/<deviceId>/cfg/forced_alert_time/ack` no-retained `{"ok":...}`
  - `ferduino/<deviceId>/cmd/forced_alert_enable` payload `"0"/"1"` o `{"enabled":true|false}` o `"get"`
  - `ferduino/<deviceId>/cfg/forced_alert_enable` retained `"0"/"1"`
  - `ferduino/<deviceId>/cfg/forced_alert_enable/ack` no-retained `{"ok":...}`
- **Paridad vs original**
  - Original: (pendiente de localizar en código legacy) — **no bloqueante** para la migración; la política de avisos diarios es decisión de producto del port.
  - Riesgo: si el reloj fuente es `millis`, el “día” se deriva por wrap de `minuteOfDay`; con RTC real será estable.


# PORTING_TRACE — Matriz de trazabilidad (Original → Port)

Este documento registra, de forma auditable, qué partes del firmware original Ferduino
se han portado al proyecto `port`, dónde están y qué diferencias existen.

## Convenciones

### Estados
- ✅ Portado (completo)
- 🟡 Parcial
- ⏳ Pendiente
- 🚫 Descartado (con justificación)

### Comentarios en código portado (obligatorio a partir de ahora)
En cada unidad portadas, añadir cabecera:

- `// [PORT] src: <ruta_original>::<función>`
- `// [PORT] dst: <ruta_port>::<función/clase>`
- `// [PORT] behavior: <qué hace>`
- `// [PORT] diffs: <diferencias vs original>`
- `// [PORT] nvm: <keys usadas>`
- `// [PORT] mqtt: <topics usados>`
- `// [PORT] flags: <APP_ENABLE_*/APP_*_USE_*>`

Esto permite auditoría automática con `rg "\[PORT\]"`.

---

## Flags (habilitar/deshabilitar módulos y backends)

### Flags de habilitación (módulo ON/OFF, útiles para debug/testing)
> Objetivo: poder aislar módulos sin tocar código, solo build flags.

- `APP_ENABLE_SENSORS` (default 1)
- `APP_ENABLE_TEMPCTRL` (default 1)
- `APP_ENABLE_SCHEDULER` (default 1)
- `APP_ENABLE_EVENTS_SCHEDULER` (default 1)
- `APP_ENABLE_TELEMETRY` (default 1)
- `APP_ENABLE_TELEMETRY_TEMPCTRL` (default 1)

> Nota: algunos aún no existen en código; se añaden cuando el módulo lo necesita. Se documentan aquí desde ya.

### Flags de backend (FAKE/REAL)
- `APP_SENSORS_USE_HW` (0=FAKE, 1=HW)
- `APP_SCHEDULER_USE_RTC` (0=millis FAKE, 1=RTC real por hook)
- `APP_OUTLETS_USE_RELAY_HAL` (0=stub, 1=HAL relés)
- `APP_TEMPCTRL_USE_GPIO` (0=solo cálculo, 1=GPIO real heater/chiller)

---

## Tabla de trazabilidad (Original → Port)

| ID | Unidad | Origen (Original) | Destino (Port) | Estado | Inputs | Outputs | NVM keys | MQTT/HA | Enable flags | Backend flags | Diffs / notas |
|---:|---|---|---|---|---|---|---|---|---|---|---|
| B6.1a | Sensores (Temperatura) | N/A (FAKE) | `src/app/sensors/sensors.cpp` | ✅ | millis/tick | `water_c`, `air_c`, `water_valid`, `air_valid` | N/A | `ferduino/<id>/telemetry/temps` | `APP_ENABLE_SENSORS` | `APP_SENSORS_USE_HW` | FAKE determinista; HW se activará sin reescritura cambiando backend |
| C1.x | Scheduler base | N/A (infra) | `src/app/scheduler/app_scheduler.cpp` | ✅ | millis/RTC | minute tick + time source | N/A | N/A | `APP_ENABLE_SCHEDULER` | `APP_SCHEDULER_USE_RTC` | RTC real será un hook/driver; lógica no cambia |
| C1.2 | Scheduler windows persistente | N/A (infra) | `src/app/scheduler/app_event_scheduler.cpp` | ✅ | minuteOfDay + config | `desiredOn(ch)` + changed flags | `330..358` | `cmd/schedule/<n>` + `cfg/schedule/<n>` (si está implementado) | `APP_ENABLE_EVENTS_SCHEDULER` | `APP_SCHEDULER_USE_RTC` | Persistencia TLV; ventana cruza medianoche soportada |
| C2 | Arbitraje Outlets Auto/Manual | Original: outlets + timers (pendiente localizar) | `src/app/outlets/app_outlets.cpp` + runtime apply | ✅ | manual cmd + desired scheduler | relay state (stub/HAL) | `310..319`, `320..329` | `cmd/outlet/<n>`, `cmd/outlet_auto/<n>` | `APP_ENABLE_EVENTS_SCHEDULER` | `APP_OUTLETS_USE_RELAY_HAL` | Manual override fuerza auto=0; scheduler solo aplica si auto=1 |
| C2.1 | Control temperatura (heater/chiller) | `Ferduino_Aquarium_Controller/src/Modules/Parametros.h::checkTempC()` | `src/app/temp_control/temp_control.cpp` | ✅ | `sensors.water_c` + cfg + outlets[0..1] | `heater_on`, `chiller_on`, `alarm_active` | `100..102` | `telemetry/tempctrl` | `APP_ENABLE_TEMPCTRL`, `APP_ENABLE_SENSORS` | `APP_TEMPCTRL_USE_GPIO` | Paridad: overrides manuales (auto/on/off) mapeados a `app::outlets` idx 0..1; mutual exclusion + safety solo en AUTO (igual que original). Fan control no portado |
| C2.1b | Telemetría tempctrl | N/A (infra) | `src/app/runtime/app_tempctrl_telemetry.cpp` | 🟡 | `tempctrl::last()` | MQTT JSON x10 | N/A | `ferduino/<id>/telemetry/tempctrl` | `APP_ENABLE_TELEMETRY_TEMPCTRL` | N/A | JSON sin floats; pensado para HA / debug |
| B6.x | MQTT config admin | N/A (infra) | `src/app/config/app_config_mqtt_admin.cpp` | ✅ | cmd mqtt | cfg persistida | `200..215` | `cmd/config` | `APP_ENABLE_TELEMETRY` (si aplica) | N/A | Permite modificar cfg sin recompilar |

---

## Pendientes (riesgo de “código fantasma”)

Lista de áreas del original que requieren mapeo explícito (por localizar y registrar):

- Timers/schedules originales (si existen más allá del scheduler nuevo).
- Dosificadora: lógica de ejecución y condiciones (por ahora solo migración NVM).
- UI/menús, logs, alarm manager, persistencias no detectadas, calibraciones.

Regla: **cada vez que portamos algo**, se añade/actualiza una fila y se anotan diffs.
