# Pinmap original Ferduino (Arduino Mega 2560)

Este documento recoge el mapeo de pines del diseño original de Ferduino
a partir del esquemático.

Objetivo:
- Entender qué usa cada pin
- Evitar conflictos en el futuro
- Servir como base para la nueva shield

---

## Buses principales

### SPI
Usado por:
- Ethernet
- RF
- SD
- TFT
- Touch

Pines SPI del Mega:
- 50 → MISO
- 51 → MOSI
- 52 → SCK
- 53 → SS (general)

Chip Select identificados en el esquema:
- SS Ethernet → ?
- SS RF → ?
- SS SD → ?
- SS TFT → ?
- SS Touch → ?

---

### I2C
Usado por:
- RTC
- Expansores IO (PCF8575)

Pines:
- 20 → SDA
- 21 → SCL

Dispositivos:
- RTC → ?
- PCF8575 → ?

---

### 1-Wire
Usado por:
- Sensores de temperatura DS18B20

Pin:
- ?

---

## Salidas (relés / drivers)

- Relay 1 → ?
- Relay 2 → ?
- Relay 3 → ?
- etc...

---

## Entradas

- Nivel agua bajo → ?
- Nivel agua alto → ?
- Botones → ?

---

## Pantalla

- TFT data/control → ?
- Touch → ?

---

## RF

- RFM12B → SPI + ?
