# EEPROM map — Ferduino legacy + ferduino-next registry

## Objetivo
Mantener compatibilidad con el firmware original de Ferduino (layout fijo por offsets) y, en paralelo, habilitar un sistema nuevo de configuración versionado para ferduino-next.

## EEPROM disponible (ATmega2560)
- Total: 4096 bytes

## Layout global (ferduino-next)

| Zona | Rango (bytes) | Estado | Uso |
|------|---------------|--------|-----|
| Legacy (compat) | 0 .. 1023 | RO (no tocar) | Mantener offsets del firmware original |
| Registry (nuevo) | 1024 .. 4095 | RW | Header + TLV/kv + CRC (ferduino-next) |

Notas:
- El firmware original usa offsets hasta ~871 (y LEDs 1..~480).
- Se reserva 0..1023 para margen y estabilidad.

## Fuente del legacy
El detalle de offsets legacy se extrae del firmware original:
- `Modules/Funcoes_EEPROM.h`

En B4.4 se implementará migración selectiva (mínima primero) desde legacy hacia el registry nuevo.
