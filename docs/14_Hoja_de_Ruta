# Ferduino HAL Migration — Hoja de Ruta

## Objetivo general

Migrar Ferduino a una arquitectura desacoplada por HAL, capaz de funcionar en:

- Modo FAKE (sin hardware)
- Modo REAL (hardware)
- Sin rediseños: solo activando flags o backends

## Fase A — Infraestructura base (COMPLETA)

Objetivo: Port funcional en PlatformIO.

Incluye:

- PlatformIO Mega2560
- HAL base: network, mqtt, rtc, relay, storage
- MQTT operativo
- Backend Home Assistant
- Topics: state, availability

**Estado: ✔ COMPLETA**

## Fase B — Capa App desacoplada (FAKE)

### B4 — Persistencia (Registry TLV) ✔

- EepromRegistry (TLV)
- Migración legacy
- Flags de migración

Keys migradas relevantes (estado actual):

- 100–102: temperatura agua
- 310–319: outlets
- 500–585: dosificación

Ver: `docs/NVM_REGISTRY_KEYS_AND_EEPROM_MAP.md` y `docs/AUDIT_CODEX_B6_SUMMARY_B_G.md`.

### B5 — Comunicaciones y Home Assistant ✔

- `CommsHABackend`
- `ha_discovery`
- Topics:
  - `ferduino/<device>/state`
  - `ferduino/<device>/availability`
  - `ferduino/<device>/cmd/outlet/<n>`
- Entidades HA:
  - outlets (switch)
  - clock_* (sensores)
- LWT automático

### B6 — Runtime funcional FAKE

#### B6.1 ✔
- Sensores FAKE (temperatura) + telemetría `telemetry/temps`
- Outlets runtime + persistencia

#### B6.2 ✔
- Scheduler FAKE (millis)
- Scheduler preparado para RTC (flag + hook)

#### B6.3 ✔ (en cierre)
Auditoría y estabilización:

- Normalización FAKE/REAL por flags
- Fixes HAL MQTT y robustez
- Documentación consolidada

## Estado actual del proyecto

- Fase A: ✔
- B4: ✔
- B5: ✔
- B6.1: ✔
- B6.2: ✔
- B6.3: en cierre (B6.3c)

## Siguiente fase — Migración funcional (B7/C)

Orden recomendado:

1) Scheduler de eventos (on/off por hora)
2) Control automático de outlets
3) Dosificación automática
4) Iluminación programada
5) Configuración dinámica vía HA/MQTT

## Flags de módulos (FAKE vs REAL)

- `APP_SCHEDULER_USE_RTC`:
  - 0 = millis() FAKE
  - 1 = RTC (requiere hook `app_scheduler_rtc_minute_of_day`)

- `APP_OUTLETS_USE_RELAY_HAL`:
  - 0 = stub
  - 1 = HAL relés

- `APP_SENSORS_USE_HW`:
  - 0 = FAKE
  - 1 = HW (pendiente drivers/HAL)
