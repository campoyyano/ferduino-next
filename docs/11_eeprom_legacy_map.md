# EEPROM legacy map (Ferduino original)

Fuente principal:
- `Ferduino_Aquarium_Controller/src/Modules/Funcoes_EEPROM.h`

Placa objetivo original: AVR (Mega2560). EEPROM total: 4096 bytes.

## Convenciones (legacy)
- Muchos bloques usan un "puntero" (byte sentinel) para validar si el bloque está inicializado.
- Valores típicos:
  - `66` como sentinel de bloque válido
  - `222` como sentinel de LEDs

---

## Tabla resumida (offsets legacy)

### LEDs (curvas 5 canales)
- Sentinel: `796 == 222`
- Datos:
  - `wled`   : `1..96`   (96 bytes)
  - `bled`   : `97..192` (96 bytes)
  - `rbled`  : `193..288` (96 bytes)
  - `rled`   : `289..384` (96 bytes)
  - `uvled`  : `385..480` (96 bytes)

### Temperatura agua
- Sentinel: `693 == 66`
- Struct `tempSettings` (config_t): `EEPROM_writeAnything(482, tempSettings)`
  - Layout típico AVR: `int tempset` (2) + `byte tempoff` (1) + `byte tempalarm` (1) => 4 bytes
- Valores convertidos:
  - guardan `set/off/alarm` en décimas (x10)

### TPA (programación)
- Sin sentinel
- Hora/minuto/duración: `488, 489, 490`
- Semana (7 bytes): `491..497`

### pH reactor (PHR)
- Sin sentinel
- `508..510` (set/off/alarm) en décimas (x10)

### ORP
- Sentinel: `705 == 66`
- Struct `ORPSettings` (config_ORP): `EEPROM_writeAnything(514, ORPSettings)`
  - Layout típico: `int ORPset` (2) + `byte ORPoff` (1) + `byte ORPalarm` (1) => 4 bytes
- Nota: setORP en entero, off/alarm en entero

### pH acuario (PHA)
- Sin sentinel
- `520..522` (set/off/alarm) en décimas (x10)

### Densidad (salinidad)
- Sentinel: `707 == 66`
- `526..528`
  - `526`: setDEN almacenado como `(setDEN - 1000)`
  - `527`: offDEN
  - `528`: alarmDEN

### Dosificadoras (calibración)
- Sentinel: `532 == 66`
- Array `dosaCalib[6]` en `533` via `EEPROM_writeAnything(533, dosaCalib)`
  - `calibracao` en décimas (x10)
  - tamaño típico: 6 * 2 = 12 bytes

### Dosificadoras (config “dose_personalizada”)
- Sentinel: `706 == 66`
- Array `dosaconfig[6]` en `545` via `EEPROM_writeAnything(545, dosaconfig)`
  - `dose_personalizada` en décimas (x10)
  - tamaño típico: 6 * 2 = 12 bytes

### Dosificadoras (horarios / flags por canal)
Para cada canal `i=0..5`:
- Hora inicio: `557+i`
- Minuto inicio: `583+i`
- Semana ON (1 byte cada día, por canal):
  - Lunes:    `589+i`
  - Martes:   `595+i`
  - Miércoles:`601+i`
  - Jueves:   `607+i`
  - Viernes:  `613+i`
  - Sábado:   `619+i`
  - Domingo:  `625+i`
- Cantidad: `631+i`
- Modo ON/OFF: `637+i`
- Hora fin: `643+i`
- Minuto fin: `649+i`

### Logs (comentados en el código)
- `688`: puntero log temp agua
- `689`: puntero log pH acuario
- `690`: puntero log pH reactor
- `691`: puntero log ORP
- `692`: puntero log densidad
- `693`: puntero config temp (sentinel)

### Wave / bombas (modo nocturna / wave)
- Sentinel: `694 == 66`
- Struct `wave` en `695` via `EEPROM_writeAnything(695, wave)`
  - Campos: `byte modo`, `byte Pump1PWM`, `byte Pump2PWM`, `int periodo`, `int duracao`
  - Tamaño típico: 1+1+1+2+2 = 7 bytes (posible padding según implementación de writeAnything)

### Error TPA
- Sin sentinel
- `709`: guarda bit (bitRead/bitWrite del estado)

### Luz nocturna (min/max intensidad)
- Sentinel: `711 == 66`
- `712`: MinI
- `713`: MaxI

### Timers (5 timers)
Para `i=0..4`:
- on_minuto: `716+i`  (716..720)
- on_hora:   `721+i`  (721..725)
- off_minuto:`726+i`  (726..730)
- off_hora:  `731+i`  (731..735)
- enabled:   `736+i`  (736..740)

### Dallas sensor addresses + asociación
- Addresses (8 bytes por sensor):
  - agua:      `766..773`
  - disipador: `775..782`
  - ambiente:  `784..791`
- Sentinel: `792 == 66`
- Asociación:
  - `793`: sonda_associada_1
  - `794`: sonda_associada_2
  - `795`: sonda_associada_3

### Coolers (temperatura min/max)
- Sentinel: `797 == 66`
- Struct `coolers` en `798` via `EEPROM_writeAnything(798, coolers)`
  - Campos: `int HtempMin_t`, `int HtempMax_t` (x10)
  - Tamaño típico: 4 bytes

### tempPot (temperatura/potencia)
- Sentinel: `802 == 66`
- `804`: tempHR
- `806`: potR
- Nota: deja huecos (803,805) sin usar.

### LED predefinido
- Sentinel: `708 == 66`
- Campos: `808..824` con huecos:
  - 808 predefinido
  - 809 pre_definido_ativado
  - 810 pwm_pre_definido
  - 811 led_on_minuto
  - 813 led_on_hora
  - 815 led_off_minuto
  - 817 led_off_hora
  - 819..823 salidas b/w/rb/r/uv
  - 824 amanhecer_anoitecer

### Alimentador
- Sentinel: `710 == 66`
- Horarios (4): `825..828`
- Días (7): `829..835`
- Opciones:
  - 836 duracao_alimentacao
  - 837 desligar_wavemaker
  - 838 quantidade_alimentacao
  - 839 alimentacao_wavemaker_on_off

### Outlets (9)
- Sentinel: `840 == 66`
- `841..849`: outlets[0..8]

### Clima (nubes/relámpagos)
- Sentinel: `850 == 66`
- `851..(851+NUMERO_CANAIS-1)`: desativarNuvemCanal[]
- `857..860`: quantDuracaoMinMaxNuvem[0..3]
- `861..862`: probNuvemRelampago[0..1]
- `863`: nuvemCadaXdias
- `864..(864+NUMERO_CANAIS-1)`: desativarRelampagoCanal[]
- `870..871`: duracaoMinMaxRelampago[0..1]

---

## Observación crítica para compatibilidad
El firmware original no usa KV ni versionado: usa offsets fijos + sentinels.
Por eso en ferduino-next:
- `0..1023` queda reservado como zona legacy (RO)
- `1024..4095` se usa para registry nuevo versionado (TLV + CRC)
