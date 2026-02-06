# Glosario MQTT Legacy (Ferduino) — Fase A

Este documento describe el protocolo MQTT “legacy” y su implementación actual en `ferduino-next` (fase A).
Objetivo: **compatibilidad 1:1** con el servidor legacy (topics, framing, IDs, keys).

> Nota: Sin placa, muchos valores son placeholders (0/""/"0000"), pero **las claves y el flujo** se mantienen.

---

## 1) Broker

Configuración por `build_flags` (PlatformIO):

- `MQTT_BROKER_HOST`
- `MQTT_BROKER_PORT`
- `FERDUINO_DEVICE_ID`
- `FERDUINO_LEGACY_USERNAME`
- `FERDUINO_LEGACY_APIKEY`

---

## 2) Topics

Formato base (legacy):

- **Command (subscribe):**
  `Username/APIKEY/topic/command`

- **Response (publish):**
  `Username/APIKEY/topic/response`

Donde:
- `Username` = `FERDUINO_LEGACY_USERNAME`
- `APIKEY`   = `FERDUINO_LEGACY_APIKEY`

---

## 3) Framing y parsing del payload (command)

- Payload en **texto**.
- Separador: `,`
- Terminador: `K`
- Primer campo: `ID` (entero)

Ejemplos:
- `0,K`
- `2,0,K`
- `2,1,06,02,2026,12,34,56,1,K`

Parsing:
1) Se recibe payload.
2) Se busca `K`.
3) Se elimina la coma previa a `K` (equivalente a `inData[j-1]='\0'`).
4) Se tokeniza por comas.
5) `id = atoi(token[0])`.

---

## 4) Respuestas (response)

- Se publica JSON al topic `.../topic/response`.
- Para escrituras/configuración suele responder:
  `{"response":"ok"}`

---

# 5) Tabla de comandos (IDs 0..17)

## ID 0 — Home
**Command:** `0,K`

**Response JSON (keys legacy):**
- `theatsink` (float)
- `twater` (float)
- `tamb` (float)
- `waterph` (float)
- `reactorph` (float)
- `orp` (float)
- `dens` (float)
- `wLedW` (int)
- `bLedW` (int)
- `rbLedW` (int)
- `redLedW` (int)
- `uvLedW` (int)
- `running` (int/segundos)
- `speed` (int)
- `moonPhase` (int)
- `iluminated` (int)
- `date` (string)
- `time` (string)
- `update` (int)
- `outlet1..outlet9` (int)
- `horaNuvem` (string, p.ej. `"[0,0,0,0,0,0,0,0,0,0]"`)
- `haveraNuvem` (int)
- `haveraRelampago` (int)

---

## ID 1 — DS18B20 (asociación/lectura)
**Command (genérico):** `1,mode,...,K`

### Mode 0 (leer)
**Command:** `1,0,K` (o con más campos; se ignoran en lectura)

**Response JSON:**
- `number` (int) — número de sondas
- `p1` (float)
- `p2` (float)
- `p3` (float)
- `ap1` (int) — asociación sonda 1
- `ap2` (int)
- `ap3` (int)

### Mode != 0 (escribir asociación/config)
**Response:** `{"response":"ok"}`

---

## ID 2 — Fecha/Hora RTC
**Command (genérico):** `2,mode,...,K`

### Mode 0 (leer)
**Command:** `2,0,K`

**Response JSON:**
- `date` (string)
- `time` (string)

### Mode != 0 (escribir)
**Command esperado (legacy):**
`2,1,dd,mm,yyyy,hh,mm,ss,dow,K`

**Response:** `{"response":"ok"}`

---

## ID 3 — Scheduler nubes/tormenta
**Command:** `3,mode,...,K`

### Mode 0 (leer)
**Response JSON:**
- `status` (int)
- `hour` (int)
- `minute` (int)
- `duration` (int)
- `monday` (int)
- `tuesday` (int)
- `wednesday` (int)
- `thursday` (int)
- `friday` (int)
- `saturday` (int)
- `sunday` (int)

### Mode != 0 (escribir)
**Response:** `{"response":"ok"}`

---

## ID 4 — Test individual LEDs / valores iniciales
**Command:** `4,mode,...,K`

### Mode 0 (leer)
**Response JSON:**
- `wLedW` (int)
- `bLedW` (int)
- `rbLedW` (int)
- `redLedW` (int)
- `uvLedW` (int)

### Mode != 0 (escribir)
**Response:** `{"response":"ok"}`

---

## ID 5 — Tabla/gráfica LED (chunk)
**Command:** `5,mode,color,first_position,K` (legacy)

### Mode 0 (leer)
**Response JSON:**
- `value0..value7` (int)

### Mode != 0 (escribir)
**Response:** `{"response":"ok"}`

---

## ID 6 — Moonlight
**Command:** `6,mode,...,K`

### Mode 0 (leer)
**Response JSON:**
- `min` (int)
- `max` (int)

### Mode != 0 (escribir)
**Response:** `{"response":"ok"}`

---

## ID 7 — Ventilador LEDs
**Command:** `7,mode,...,K`

### Mode 0 (leer)
**Response JSON:**
- `minfan` (int)
- `maxfan` (int)

### Mode != 0 (escribir)
**Response:** `{"response":"ok"}`

---

## ID 8 — Reducción potencia LEDs por temperatura
**Command:** `8,mode,...,K`

### Mode 0 (leer)
**Response JSON:**
- `templimit` (int)
- `potred` (int)

### Mode != 0 (escribir)
**Response:** `{"response":"ok"}`

---

## ID 9 — Control temperatura agua
**Command:** `9,mode,...,K`

### Mode 0 (leer)
**Response JSON:**
- `setTemp` (float)
- `offTemp` (float)
- `alarmTemp` (float)

### Mode != 0 (escribir)
**Response:** `{"response":"ok"}`

---

## ID 10 — Dosificación manual
**Command (legacy):** `10,pump,dose,K`

**Response JSON:**
- `wait` (string) — tiempo estimado/estado

---

## ID 11 — Calibración dosificadoras
**Command:** `11,mode,pump,...,K`

### Mode 0 (leer)
**Response JSON:**
- `calib` (float)
- `wait` (string)

### Mode != 0 (escribir)
**Response:** `{"response":"ok"}`

---

## ID 12 — Programación dosificadoras (scheduler)
**Command:** `12,mode,pump,...,K`

### Mode 0 (leer)
**Response JSON:**
- `onoff` (int)
- `quantity` (int)
- `dose` (float)
- `hStart` (int)
- `mStart` (int)
- `hEnd` (int)
- `mEnd` (int)
- `monday` (int)
- `tuesday` (int)
- `wednesday` (int)
- `thursday` (int)
- `friday` (int)
- `saturday` (int)
- `sunday` (int)

### Mode != 0 (escribir)
**Response:** `{"response":"ok"}`

---

## ID 13 — Control pH acuario
**Command:** `13,mode,...,K`

### Mode 0 (leer)
**Response JSON:**
- `setPHA` (float)
- `offPHA` (float)
- `alarmPHA` (float)

### Mode != 0 (escribir)
**Response:** `{"response":"ok"}`

---

## ID 14 — Control pH reactor
**Command:** `14,mode,...,K`

### Mode 0 (leer)
**Response JSON:**
- `setPHR` (float)
- `offPHR` (float)
- `alarmPHR` (float)

### Mode != 0 (escribir)
**Response:** `{"response":"ok"}`

---

## ID 15 — Control ORP
**Command:** `15,mode,...,K`

### Mode 0 (leer)
**Response JSON:**
- `setORP` (float)
- `offORP` (float)
- `alarmORP` (float)

### Mode != 0 (escribir)
**Response:** `{"response":"ok"}`

---

## ID 16 — Control densidad/salinidad
**Command:** `16,mode,...,K`

### Mode 0 (leer)
**Response JSON:**
- `setDEN` (float)
- `offDEN` (float)
- `alarmDEN` (float)

### Mode != 0 (escribir)
**Response:** `{"response":"ok"}`

---

## ID 17 — Lectura `filepumpX` (SD)
**Command (legacy observado):** `17,mode,idx,K` (el original usa el token `[2]` como índice)

**Response JSON:**
- `filepump<idx>` (string)

Ejemplo:
```json
{"filepump2":"0000"}
