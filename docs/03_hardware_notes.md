# Hardware Notes (Shield Mega-form-factor)

## Objetivo
Diseñar una shield compatible físicamente con Arduino Mega form factor (Mega 2560 / Due / Giga R1) y eléctricamente portable (5V/3.3V).

## Dominios eléctricos
- VIN_Power: (p.ej. 12V)
- +5V_Power: cargas / relés (si aplica)
- +3V3_Logic: periféricos digitales (SPI/I2C, RF, ETH, FRAM, RTC)
- VIO (IOREF): referencia lógica de la placa base (5V en Mega, 3.3V en Due/Giga)

## Reglas de compatibilidad
1. Nada conecta al MCU con niveles > 3.3V.
2. Buses I2C con pull-ups a 3.3V.
3. SPI a 3.3V; si la base es Mega 5V, bajar nivel en MOSI/SCK/CS con 74LVC.
4. Entradas externas: protección ESD + limitación a 3.3V.

## Bloques

### RTC
- Opción A: DS1307 (compatibilidad legacy)
- Opción B: DS3231 (precisión)
Decisión: pendiente

### Ethernet
- SPI @3.3V
- Estrategia de suministro: footprint universal / módulo / chip
Decisión: pendiente

### RF
- Footprint: header estable + adaptador
Decisión: pendiente

### Display/Touch
- TFT SPI (ILI9341 2.8" 320x240)
- Touch: XPT2046 (SPI) / FT6206 (I2C)
Decisión: pendiente

### Outputs (Relés/MOSFETs)
- Drivers: ULN2803A / MOSFET logic-level
- Protecciones: diodo flyback / TVS / fusible
Decisión: pendiente

### Inputs (Nivel / DS18B20 / BNC)
- DS18B20: 1-Wire + pull-up 4.7k a 3.3V (o 5V con limitación)
- Nivel: pull-up 3.3V + Schmitt opcional
- BNC: acondicionamiento a 0–3.3V
Decisión: pendiente

## Mecánica
- Form factor Mega
- Referencia placement: Componentes-Ferduino-Mega-2560.pdf
Notas: pendiente

## Pendientes / Riesgos
- Definir alimentación y corrientes máximas por salida.
- Definir número exacto de relés/canales.
- Definir estrategia RF (módulo/driver) y estrategia Ethernet (módulo/chip).
- Definir stack de UI (librerías + rendimiento en Mega).

