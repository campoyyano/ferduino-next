# Ferduino-next – Firmware PORT – Contexto del Proyecto


### [2026-02-06] A9.2 – Implementación Ethernet (W5x00) + smoketest

- Se implementa `INetworkHal` para Mega2560 usando la librería `Ethernet` (W5x00) en `src/hal/impl/mega2560/`.
- Se añade `app_network_smoketest` para validar DHCP e impresión de IP (IP/GW/MASK/DNS).
- Regla mantenida: `Ethernet.h` solo aparece en la implementación específica de plataforma.


### [2026-02-06] A9.1 – Definición de la HAL de red

- Se introduce la interfaz pública `hal_network.h`.
- Se define el contrato de la HAL de red (Ethernet / futuros transports).
- No se incluyen dependencias de plataforma (`Ethernet.h`, etc.).
- La implementación se realizará en `src/hal/impl/<plataforma>/`.


## Estado actual del port (Firmware/port)

- El port es un proyecto PlatformIO limpio, desacoplado del firmware original de Ferduino.
- `Firmware/Original` se mantiene únicamente como referencia y **no debe incluirse** desde el port.
- Tras la reorganización de carpetas, el proyecto compila correctamente.

### Organización del código

- `Firmware/port/include/hal/`  
  Interfaces públicas de la HAL (sin includes dependientes de plataforma).

- `Firmware/port/src/hal/impl/mega2560/`  
  Implementaciones concretas de la HAL para Arduino Mega 2560.

- `Firmware/port/src/app/smoketests/`  
  Pruebas de integración simples para validar cada HAL de forma aislada.

- `Firmware/port/src/main.cpp`  
  Punto de entrada del firmware (selección de smoketests o lógica de aplicación).


## Objetivo general
Portar el firmware original del Ferduino (Arduino Mega 2560) a una arquitectura
moderna basada en HAL, manteniendo compatibilidad con el hardware original
antes de migrar a otras plataformas (Giga, ESP, etc).

La prioridad actual es:
1. Que compile
2. Que funcione en el hardware original
3. Que la abstracción HAL sea limpia y extensible

---

## Hardware actual objetivo
- Board principal: **Arduino Mega 2560**
- IO Expander: **PCF8575**
- RTC original: **DS1307**
- RTC alternativo soportado: **DS3231**
- LEDs PWM directos desde el Mega
- Relés controlados vía PCF8575

---

## Decisiones de diseño importantes

### HAL
- Todo acceso a hardware pasa por HAL
- No se accede directamente a Wire / digitalWrite / pinMode desde la app
- HAL es C++ ligero, sin dependencias innecesarias

### IO Expander
- Se usa un HAL genérico (`IoExpander`) con:
  - Dirección configurable
  - Soporte ACTIVE_LOW
  - Shadow register interno
- Implementación basada en I2C puro (no dependencia directa de librerías externas)

### RTC
- HAL RTC soporta **DS1307 y DS3231**
- API unificada (`FechaHora`)
- Selección por disponibilidad en runtime
- RTC ya compilando correctamente

---

## Estado actual (cerrado)

### HAL completados
- hal_gpio
- hal_time
- hal_i2c
- hal_pwm
- hal_ioexpander (PCF8575)
- hal_rtc (DS1307 + DS3231)

### Smoke tests existentes y compilando
- app_gpio_smoketest
- app_ferduino_pins_smoketest
- app_ioexpander_smoketest
- app_pwm_leds_smoketest
- app_rtc_smoketest

---

## Pendiente / siguientes pasos

### HAL pendientes
- hal_relay (encima de IoExpander)
- hal_network (Ethernet / MQTT) – NO prioritario aún

### A corto plazo
1. Implementar `hal_relay`
2. Smoke test de relés
3. Validar en hardware real (Mega original)
4. Limpieza final de includes y dependencias

---

## Notas importantes
- El repositorio es la fuente de verdad
- No se deben subir ZIPs ni `.pio`
- Archivos grandes NUNCA al repo (GitHub limit)
