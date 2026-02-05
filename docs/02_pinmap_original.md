# Pinmap original Ferduino (Arduino Mega 2560)

Este documento recoge el mapeo de pines del diseño original Ferduino.

## Fuentes (orden de prioridad)
1) **Firmware / header (.h)**: define los pines con `const byte ... = <pin>;` → **fuente primaria (verdad operativa del código)**  
2) **PinMap.pdf**: conectores externos y algunas señales → **fuente secundaria**  
3) **Esquemático PDF**: validación de discrepancias y señales no cubiertas → **fuente de verificación**

---

## Objetivo
- Evitar conflictos de pines en el rediseño (shield Fase 2).
- Tener una referencia única para HAL/portabilidad (Mega/Due/Giga).
- Documentar qué está ocupado, qué es bus, y qué es “libre”.

---

## Resumen de buses y reservas “grandes”
### SPI (Mega)
- D50 → MISO
- D51 → MOSI
- D52 → SCK

**Chip selects (según firmware/header):**
- Ethernet SS → D53
- RFM12B SS → A15 (D69)
- SD CS → D5 *(nota: configurable en código, ver sección SD)*

### I2C (Mega)
- D20 → SDA
- D21 → SCL

Usado por:
- PCF8575 (expansor de salidas)
- RTC (en el diseño original)

### TFT / LCD (paralela)
- D22..D41 → reservados para TFT/LCD paralelo (bus grande)

### Touch
- D3..D7 → reservados para touch (nota: D5 además SD CS)

### RF
- D2 → interrupción RFM12B (INT)

### 1-Wire / Temperatura
- D49 → `sensoresPin` (típico bus DS18B20)

---

## Pinmap Mega (definiciones directas del firmware)

### Digital (GPIO)
| Pin Mega | Nombre en firmware | Función / notas |
|---:|---|---|
| D0 | `alarmPin` | Alarma (ojo: Serial0 RX) |
| D1 | `desativarFanPin` | Desactiva coolers (ojo: Serial0 TX) |
| D2 | (reservado) | RFM12B interrupt (INT) |
| D3 | (reservado) | Touch |
| D4 | (reservado) | Touch |
| D5 | (reservado) | SD CS + Touch |
| D6 | (reservado) | Touch |
| D7 | (reservado) | Touch *(nota en código: libre en DIY; usado en Ferduino M)* |
| D8 | `ledPinUV` | LED UV (PWM) |
| D9 | `ledPinBlue` | LED Blue (PWM) |
| D10 | `ledPinWhite` | LED White (PWM) |
| D11 | `ledPinRoyBlue` | LED Royal Blue (PWM) |
| D12 | `ledPinRed` | LED Red (PWM) |
| D13 | `fanPin` | Ventiladores (PWM) |
| D16 | `multiplexadorS0Pin` | Multiplexador S0 |
| D17 | `multiplexadorS1Pin` | Multiplexador S1 |
| D20 | (bus) | I2C SDA |
| D21 | (bus) | I2C SCL |
| D22..D41 | (reservado) | TFT/LCD paralelo |
| D42 | `aquecedorPin` | Heater |
| D43 | `chillerPin` | Chiller |
| D44 | `ledPinMoon` | Luz lunar |
| D45 | `wavemaker1` | Wavemaker 1 |
| D46 | `wavemaker2` | Wavemaker 2 |
| D49 | `sensoresPin` | Bus sensores temp (típico DS18B20) |
| D50 | (bus) | SPI MISO |
| D51 | (bus) | SPI MOSI |
| D52 | (bus) | SPI SCK |
| D53 | (CS) | Ethernet SS |

> Nota importante: D0/D1 se usan en firmware pero son Serial0. Para una shield nueva, conviene evitar usarlos salvo que se asuma que Serial0 no se usa (o se redirija debug a otro puerto/USB).

---

### Analog (ADC / GPIO)
| Pin | Nombre en firmware | Función / notas |
|---:|---|---|
| A0 (D54) | `sensor1` | Sensor entrada 1 (nivel/boya, etc.) |
| A1 (D55) | `sensor2` | Sensor entrada 2 |
| A2 (D56) | `sensor3` | Sensor entrada 3 |
| A3 (D57) | `sensor4` | Sensor entrada 4 |
| A4 (D58) | `sensor5` | Sensor entrada 5 |
| A5 (D59) | `sensor6` | Sensor entrada 6 |
| A9 (D63) | `dosadora1` | Dosificadora 1 |
| A10 (D64) | `dosadora2` | Dosificadora 2 |
| A11 (D65) | `dosadora3` | Dosificadora 3 |
| A12 (D66) | `dosadora4` | Dosificadora 4 |
| A13 (D67) | `dosadora5` | Dosificadora 5 |
| A14 (D68) | `dosadora6` | Dosificadora 6 |
| A15 (D69) | (CS) | RFM12B SS |

---

## Expansor I2C PCF8575 (salidas remotas)
El firmware define las señales como índices P0..P12 del PCF8575:

| PCF8575 bit | Nombre firmware | Descripción |
|---:|---|---|
| P0 | `ozonizadorPin` | Ozonizador |
| P1 | `reatorPin` | CO2 reactor |
| P2 | `bomba1Pin` | Bomba 1 (quarentena) |
| P3 | `bomba2Pin` | Bomba 2 (sump out) |
| P4 | `bomba3Pin` | Bomba 3 (sump in) |
| P5 | `temporizador1` | Timer 1 |
| P6 | `temporizador2` | Timer 2 |
| P7 | `temporizador3` | Timer 3 |
| P8 | `temporizador4` | Timer 4 |
| P9 | `temporizador5` | Timer 5 |
| P10 | `solenoide1Pin` | Solenoide (reposición agua dulce) |
| P11 | `alimentadorPin` | Alimentador automático |
| P12 | `skimmerPin` | Skimmer |

## PCF8575 (I2C expander) - Addressing

- Address range: 0x20..0x27 via A2:A1:A0
- Default address: 0x20 (A2=A1=A0=0)
- Implementation: each Ax has 100k pulldown to GND + solder jumper to VCC (3.3V) to set '1'
- I2C pull-ups: 4.7k to 3.3V on SDA/SCL, enabled via solder jumper SJ_I2C_PU

---

## Conectores externos (según PinMap.pdf)

### DB9 (LEDS)
- D8
- D9
- D10
- D11
- D12
- D44
- GND

### RJ11 (COOLERS)
- D13
- D1
- GND

### RJ11 (WAVEMAKER)
- D45
- D46
- 5V
- GND

### Jack 3.5mm (DS18B20)
- DATA: D49 (`sensoresPin`)
- VCC + GND

### DB25 (RELAYS-2 / PCF8575)
- P0..P15 (PCF8575) hacia el conector
- Incluye líneas de GND, 5V y VIN (según PinMap)

---

## TFT / Touch (detalle de firmware)
- El firmware define `USE_TFT` y un modelo: `TFT_MODEL ITDB32WD`.
- Nota en código: normalmente debería funcionar para modelos con controlador **HX8352-A**.
- Pins reservados:
  - TFT paralelo: D22..D41
  - Touch: D3..D7 (D5 compartido con SD CS)

> Pendiente: detallar exactamente DB0..DB15 + señales RS/WR/RD/RST dentro del rango D22..D41 a partir del esquemático.

### Selección de CS para SD (según Configuration.h)
- Por defecto: SD_CS = D5 (compartido con touch reservado)
- Opción Ethernet shield: CS = D4 si se activa USE_PIN_4_FOR_SD_CARD
- Opción TFT shield: CS = D53 si se activa USE_PIN_53_FOR_SD_CARD (NO usar en Ferduino Mega 2560 según comentario del autor)
- SD_CARD_SPEED por defecto: SPI_HALF_SPEED

---

## SD Card (punto crítico)
Por defecto en el firmware:
- **SD CS = D5** (y D5 está también reservado para touch).

El firmware incluye opciones comentadas:
- Usar SD en **ethernet shield** (indicando un pin alternativo en el código).
- Usar SD en **TFT** (hay una opción mencionada pero con advertencias).

Acción en el rediseño:
- En la shield nueva, definir claramente **una sola SD** y su CS fijo (idealmente un CS dedicado que no comparta con touch).

---

## Notas / riesgos detectados
1) **D0/D1** usados por firmware → conflicto potencial con Serial0 (debug/programación).
2) **D5 compartido** (SD CS + touch reservado) → posible fuente de bugs si se cambia el stack de pantalla/SD.
3) TFT ocupa un bloque enorme de pines → justifica totalmente la migración a **TFT SPI** en la Fase 2.

---

## Pendientes (verificación contra esquemático)
- Dirección I2C y cableado del PCF8575.
- Señales exactas TFT dentro de D22..D41 (DBx + control).
- Qué tipo de touch exacto (resistivo analógico vs controlador SPI) y si se usa SPI real o solo pines dedicados.
- Confirmar si hay alguna segunda SD (por módulos/shields) en el diseño original.
