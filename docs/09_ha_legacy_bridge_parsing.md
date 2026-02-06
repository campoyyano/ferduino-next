# Bridge Legacy → Home Assistant (B2.1) — Documento de parseo

Este documento define cómo se convierte el “estado legacy” (Ferduino original) a un `state` MQTT compatible con Home Assistant.
Diseñado para AVR (Mega2560): sin parser JSON completo, con extracción ligera por claves conocidas.

---

## 1) Objetivo

- Mantener el backend legacy como **fuente de verdad**.
- Generar un `state` HA derivado y publicarlo a:
  - `ferduino/<device_id>/state`
- Evitar duplicación de lógica: HA consume estado; legacy produce estado.

---

## 2) Entrada (origen): JSON legacy

El origen principal es el JSON del comando **ID 0 (Home)** en el protocolo legacy.

Ejemplo (parcial):

```json
{
  "theatsink": 0.0,
  "twater": 0.0,
  "tamb": 0.0,
  "waterph": 0.0,
  "reactorph": 0.0,
  "orp": 0.0,
  "dens": 0.0,
  "wLedW": 0,
  "bLedW": 0,
  "rbLedW": 0,
  "redLedW": 0,
  "uvLedW": 0,
  "running": 1234,
  "outlet1": 0,
  "outlet2": 1
}

