# Arquitectura por bloques — Shield ferduino-next (Fase 2)

Objetivo:
Diseñar una shield compatible con el form factor Arduino Mega (Mega/Due/Giga) y basada en buses (I2C, SPI, 1-Wire), reduciendo al mínimo el cableado directo por pines.

Fuentes:
- Firmware (Configuration.h) = verdad operativa de pines y opciones.
- Esquema original = referencia de bloques y señales.
- PinMap.pdf = conectores externos.

---

## Bloques del sistema (visión general)

### 1) Alimentación y dominios eléctricos
Dominios:
- VIN_Power (por definir: típicamente 12V desde fuente externa)
- +5V_Power (cargas / relés / periféricos que lo requieran)
- +3V3_Logic (toda la lógica y periféricos digitales)
- VIO = IOREF de la placa base (Mega=5V, Due/Giga=3.3V)

Referencia del diseño original:
- Regulador 5V tipo NCP1117
- Regulador 3.3V tipo AMS1117-3.3V
- Existía distribución de 3.3V y 5V en el esquema original.  

Notas:
- En la shield nueva, la regla es: lógica a 3.3V siempre.
- La potencia se aisla/driveriza para aceptar control desde 3.3V.

---

### 2) MCU base (Arduino) + cabeceras estándar
Placas objetivo:
- Arduino Mega 2560 (5V)
- Arduino Due / Giga (3.3V)

Regla:
- Nunca enviar >3.3V a un pin de MCU (por compatibilidad Due/Giga).

---

### 3) Bus I2C (expansión)
Usos:
- PCF8575 (expansor principal de salidas)
- RTC (original: DS1307)
- FRAM futura (opcional)

Pines:
- SDA D20, SCL D21

Dirección PCF8575:
- Rango 0x20..0x27 con A0/A1/A2.
- Implementación: A0/A1/A2 con pulldown + solder jumper a 3.3V (configurable).

Pull-ups I2C:
- Pull-ups a 3.3V con posibilidad de desconexión por solder jumper (para evitar duplicados).

---

### 4) Bus SPI (periféricos)
Usos (objetivo Fase 2):
- Ethernet
- RF
- SD
- Pantalla SPI futura

Pines:
- MISO D50, MOSI D51, SCK D52

Chip selects (concepto):
- CS_ETH
- CS_RF
- CS_SD
- CS_TFT_SPI (futuro)

Compatibilidad 5V/3.3V:
- Cuando la base sea Mega (5V), bajar nivel en SCK/MOSI/CS usando 74LVC (o equivalente) alimentado a 3.3V.

---

### 5) Ethernet
Original:
- Controlador W5100 (SPI).  

Decisión Fase 2:
- Mantener Ethernet por SPI inicialmente (por compatibilidad con el código).
- En el futuro permitir cambio a módulo (por disponibilidad), pero manteniendo la interfaz SPI.

---

### 6) RF
Original:
- Módulo RFM12B 433 MHz (SPI + interrupción).  

Decisión Fase 2:
- Mantener RF como módulo enchufable (header) para mitigar riesgo de supply.
- Mantener INT y CS dedicados.

---

### 7) RTC
Original:
- DS1307 (I2C).  

Decisión Fase 2:
- Mantener DS1307 como baseline por compatibilidad.
- Permitir footprint alternativo (opcional) a DS3231 en el futuro (misma interfaz I2C).

---

### 8) Temperatura (1-Wire)
Original:
- Bus de temperatura (DS18B20) presente como bloque separado.

Decisión Fase 2:
- Mantener 1-Wire.
- Conector dedicado y protección ESD mínima.

---

### 9) Salidas de potencia (relés / bombas / válvulas / etc.)
Original:
- Drivers tipo ULN2803 / ULN2003 en el esquema.
- Varias salidas parecen colgar del PCF8575.

Decisión Fase 2:
- Mantener “lógica a 3.3V” y diseñar etapa de potencia que acepte 3.3V.
- Separar potencia (5/12V) de lógica y definir protecciones (flyback/TVS/fusibles) cuando aplique.

---

### 10) Pantalla / UI
Original:
- TFT/LCD paralela consumiendo D22..D41 (muy costoso en pines).
- Touch reservado en D3..D7.
- En el esquema original se ven señales LCD_* y señales de touch + SD en el bloque TFT.

Decisión Fase 2:
- Objetivo: migrar a pantalla SPI para recuperar pines y simplificar PCB.
- Mantener en documentación qué pines se liberan y qué nuevas señales SPI aparecen.

---

## Reglas de diseño (para no romper compatibilidad)
1) Periféricos digitales a 3.3V.
2) Level shifting solo donde sea necesario (Mega->periférico).
3) I2C siempre con pull-ups a 3.3V.
4) PCF8575 direccionable por jumpers (A0/A1/A2).
5) Footprints “arriesgados” (RF/Ethernet) preferiblemente modulares o con alternativa.

---

## Pendientes cerrados por documentación (antes de KiCad)
- Definir VIN_Power (12V típico) y corrientes máximas.
- Definir número exacto de canales de potencia (relés/MOSFET) que replicaremos del original.
- Decidir estrategia Ethernet (chip vs módulo) y RF (módulo fijo).
- Decidir pantalla SPI concreta (controlador) para planificar CS/DC/RST.