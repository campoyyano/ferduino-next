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
- SS Ethernet → D53
- SS RF (SS RFM12) → A15
- SS SD (SD CHIP SELECT) → D5
- SS TFT → (pendiente)
- SS Touch → D3..D7 (TOUCH SCREEN usa D3 hasta D7)

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
- DATA: (pendiente pin Mega)
- VCC + GND

---

## Pantalla TFT (control)
- TFT_RS → A0
- TFT_WR → A1
- TFT_RD → A2
- TFT_RESET → A3
- SD_PRESENT → A4
- LED_PIN → A5
- TFT D/C (control) → A8

Bus SPI compartido:
- MISO → D50
- MOSI → D51
- SCK  → D52

---

## RF (RFM12B)

- Interrupt → D2 (RFM12B INTERRUPT)
- SS (CS) → A15 (SS RFM12)
- SPI bus → D50/D51/D52