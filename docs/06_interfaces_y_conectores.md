# Interfaces y conectores — Shield ferduino-next (Fase 2)

Objetivo:
Definir el "contrato físico" de la shield: conectores, señales y alimentación asociada.
Este documento debe cerrarse antes de empezar el PCB.

Fuentes:
- PinMap.pdf (conectores y pines)
- Firmware (Configuration.h) para corroborar funciones
- Esquema original para ver implementación eléctrica (drivers/protecciones)

---

## Conectores externos (heredados del diseño original)

### 1) DB9 — LEDS
Uso: canales PWM de iluminación + moonlight

Señales:
- D8  : LED UV
- D9  : LED Blue
- D10 : LED White
- D11 : LED Royal Blue
- D12 : LED Red
- D44 : Moonlight
- GND

Notas:
- Estos pines coinciden con el pinout del firmware para iluminación.
- Definir si los canales son PWM directos o si van a drivers externos.

---

### 2) RJ11 — COOLERS
Uso: ventiladores / control de coolers

Señales:
- D13 : fanPin (PWM)
- D1  : desativarFanPin (nota: D1 es Serial0 TX en Mega)
- GND

Notas:
- En la shield nueva se recomienda NO usar D0/D1 si queremos mantener Serial0 libre.
- Si se mantiene compatibilidad: documentar claramente el impacto.

---

### 3) RJ11 — WAVEMAKER
Uso: control wavemaker

Señales:
- D45 : wavemaker1
- D46 : wavemaker2
- +5V
- GND

Notas:
- Confirmar tipo de salida (PWM/ON-OFF) y etapa de potencia asociada.

---

### 4) Jack 3.5 mm — DS18B20 (temperatura)
Uso: bus 1-Wire

Señales:
- DATA : D49 (sensoresPin)
- VCC
- GND

Notas:
- Pull-up a 3.3V en la shield nueva (regla 3.3V lógica).
- Añadir protección ESD mínima en el conector.

---

### 5) DB25 — RELAYS-2 / PCF8575
Uso: salidas desde expansor I2C PCF8575

Señales:
- P0..P15 (PCF8575 bits 0..15)
- Incluye GND, 5V y VIN (según PinMap)

Notas:
- En el firmware se usan al menos P0..P12 (ozonizador, reactor, bombas, timers, solenoide, alimentador, skimmer).
- Dejar preparada la arquitectura para usar P13..P15 en futuras ampliaciones.

---

## Conectores internos / de servicio (propuestos Fase 2)

### A) Header I2C (opcional)
Uso: expansión (sensores, FRAM, etc.)

Señales:
- SDA (D20)
- SCL (D21)
- 3.3V
- GND

Notas:
- Pull-ups a 3.3V con solder-jumper (SJ_I2C_PU)

---

### B) Header SPI (opcional)
Uso: depuración / módulos SPI

Señales:
- MISO (D50)
- MOSI (D51)
- SCK  (D52)
- 3.3V
- GND
- CS libres (definidos por diseño)

---

### C) Conector de alimentación
Pendiente:
- Definir si la shield recibirá:
  - VIN (12V) y generará 5V/3.3V
  - o recibirá 5V ya regulado

Notas:
- Separar claramente potencia y lógica.
- Añadir fusible / protección contra polaridad si alimentamos desde VIN.

---

## Decisiones pendientes (antes de PCB)
1) ¿Se mantienen D0/D1 o se reasignan (recomendado reasignar)?
2) Definir etapas de salida: PWM directo vs drivers integrados.
3) Definir estándar físico exacto de los conectores (tipo de RJ11, DB9/DB25, jack).
4) Definir ubicación en PCB usando el placement original como referencia.

## Decisión sobre D0/D1 (Serial0) — Perfil legacy vs perfil Fase 2

Motivo:
En Arduino Mega, D0/D1 corresponden a Serial0 (RX0/TX0). Usarlos para señales de control puede interferir con:
- programación,
- monitor serie,
- depuración.

Decisión:
- Se implementarán 2 perfiles de pines:
  - Perfil LEGACY (compatibilidad total): mantiene alarmPin=D0 y desativarFanPin=D1.
  - Perfil FASE2 (recomendado): reasigna alarmPin y desativarFanPin para liberar Serial0.

Propuesta de pines alternativos (FASE2):
- alarmPin → D14
- desativarFanPin → D15

Notas:
- D14/D15 no chocan con SPI (50–53), I2C (20–21), TFT paralela (22–41), Touch (3–7), 1-Wire (49) ni con los PWM usados por LEDs (8–12) y fan (13).
- Si en el futuro se usa Serial3, se podrá cambiar esta asignación sin romper el perfil legacy.
