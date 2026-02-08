# PORTING PLAN B5/B6 - ferduino-next

- Fecha: 2026-02-08
- Commit base: `b29ef64`
- Alcance del documento: planificación técnica incremental (sin cambios funcionales)

## Comandos usados durante el análisis

```powershell
git rev-parse --short HEAD
Get-Date -Format "yyyy-MM-dd"
rg --files Firmware/port/include Firmware/port/src
rg -n "class|namespace|publishDiscovery|bridgeFromLegacyHomeJson|migrateLegacyIfNeeded|REGF_MIGRATED|StorageRegion|INetworkHal|IMqttHal|EepromRegistry|comms_legacy|comms_ha|SMOKETEST|src_filter|build_flags" Firmware/port/include Firmware/port/src Firmware/port/platformio.ini
rg --files Firmware/Original | rg -n "Ferduino_Aquarium_Controller/src|Modules|RTC|EEPROM|Temperatura|Dallas|OneWire|Dosing|Dos|Wave|PWM|LED|UTFT|URTouch|Touch|Rele|Relay|MQTT|Ethernet|Funcoes|Parametros|Timer|Aq"
Get-ChildItem -Path Firmware/Original/Ferduino_Aquarium_Controller/src -Recurse -File | Select-Object FullName
```

## A) Estado actual resumido

### HAL (implementado)

- Red:
  - Contrato: `Firmware/port/include/hal/hal_network.h` (`INetworkHal`, `ITcpClient`)
  - Implementación Mega/W5x00: `Firmware/port/src/hal/impl/mega2560/hal_network_ethernet_w5x00.cpp`
- MQTT:
  - Contrato: `Firmware/port/include/hal/hal_mqtt.h` (`IMqttHal`)
  - Implementación PubSubClient: `Firmware/port/src/hal/impl/mega2560/hal_mqtt_pubsubclient.cpp`
- Storage:
  - Contrato: `Firmware/port/include/hal/hal_storage.h` (`IStorageHal`)
  - Implementación EEPROM AVR: `Firmware/port/src/hal/impl/mega2560/hal_storage_eeprom.cpp`
  - `Log` y `Archive` aún sin backend (capacidad 0)
- RTC/I2C/IO expander/relay/PWM/serial:
  - `Firmware/port/src/hal/impl/mega2560/hal_rtc.cpp`
  - `Firmware/port/src/hal/impl/mega2560/hal_i2c.cpp`
  - `Firmware/port/src/hal/impl/mega2560/hal_ioexpander.cpp`
  - `Firmware/port/src/hal/impl/mega2560/hal_relay.cpp`
  - `Firmware/port/src/hal/impl/mega2560/hal_pwm.cpp`
  - `Firmware/port/src/hal/impl/mega2560/hal_serial.cpp`

### app/comms (implementado)

- Selector backend runtime: `Firmware/port/src/app/comms/comms.cpp`
- Backend legacy MQTT:
  - `Firmware/port/src/app/comms/comms_legacy_backend.cpp`
  - Soporta parsing comando legacy (IDs 0..17), topics `username/apikey/topic/*`
- Backend HA MQTT:
  - `Firmware/port/src/app/comms/comms_ha_backend.cpp`
  - Soporta state/availability/cmd/outlet/cmd/config
- Bridge + discovery HA:
  - `Firmware/port/src/app/comms/ha/ha_legacy_bridge.cpp`
  - `Firmware/port/src/app/comms/ha/ha_discovery.cpp`
  - `Firmware/port/src/app/comms/ha/ha_outlet_control.cpp`

### NVM (implementado)

- Layout dual EEPROM:
  - `Firmware/port/include/app/nvm/eeprom_layout.h`
  - Legacy reservado `0..1023`, registry `1024..4095`
- Registry TLV + CRC:
  - `Firmware/port/include/app/nvm/eeprom_registry.h`
  - `Firmware/port/src/app/nvm/eeprom_registry.cpp`
- Migración subset + flags:
  - `Firmware/port/src/app/nvm/eeprom_migration.cpp`
  - Migra offsets legacy `482/484/485` a keys `100/101/102`
  - Flag `REGF_MIGRATED`

### Qué falta para empezar porting real del original

- Runtime principal orientado a smoketest, no a flujo app completo (`Firmware/port/src/main.cpp`).
- `app::cfg` persiste aún por EEPROM directa (`Firmware/port/src/app/config/app_config.cpp`), no por registry TLV.
- IDs legacy mayoritariamente con placeholders (estructura correcta, lógica real pendiente).
- `ha_outlet_control` en RAM; no acciona relés físicos todavía.
- Falta “modelo interno único de estado” para evitar duplicación legacy/HA.

## B) Estrategia general de porting (filosofía)

### Strangler pattern (recomendado)

1. Mantener backend legacy operativo (compatibilidad externa).
2. Encapsular cada módulo portado detrás de APIs nuevas (`hal/*`, `app/*`).
3. Reemplazar llamadas al código legacy módulo a módulo.
4. Mantener tests/smoketests y build verde en cada paso.

### Preprocesador y reducción de superficie

- Feature flags por dominio (ejemplo):
  - `-D FEAT_SENSORS_DS18B20=1`
  - `-D FEAT_DOSING=0`
  - `-D FEAT_UI_UTFT=0`
- `src_filter` por entorno PlatformIO para excluir módulos no listos.
- Targets separados:
  - `mega2560_port` (mínimo hardware + smoke)
  - futuros `due_port`, `giga_port` con HAL específica
- Regla: features desactivadas deben compilar sin código muerto roto.

### Criterios de “módulo portado”

- API nueva establecida y documentada.
- Smoketest mínimo ejecutable sin placa (simulación de flujo + asserts/logs).
- Sin dependencias cruzadas no controladas a `Firmware/Original`.
- En AVR evitar heap/`String` siempre que sea viable.
- Publicación de estado por comms legacy/HA desde una sola fuente interna.

## C) Backlog B5 (config runtime desde registry)

### B5.1 - Leer MQTT/Network desde registry con fallback a build_flags

- Archivos a tocar:
  - `Firmware/port/src/app/config/app_config.cpp`
  - `Firmware/port/include/app/config/app_config.h`
  - `Firmware/port/include/app/nvm/eeprom_registry.h` (o nuevo `Firmware/port/include/app/nvm/registry_keys.h`)
  - `Firmware/port/src/app/nvm/eeprom_migration.cpp`
- Keys sugeridas (TLV u16):
  - `1000` backend_mode (u8)
  - `1001` mqtt_host (str)
  - `1002` mqtt_port (u16/u32)
  - `1003` mqtt_device_id (str)
  - `1010` net_use_dhcp (u8)
  - `1011` net_mac (blob[6])
  - `1012` net_ip (blob[4])
  - `1013` net_gw (blob[4])
  - `1014` net_mask (blob[4])
  - `1015` net_dns (blob[4])
- Criterios de aceptación (sin placa):
  - `pio run -d Firmware/port -e mega2560_port` compila.
  - Al iniciar con registry vacío: carga defaults de build flags.
  - Tras guardar config y reiniciar (simulado), se rehidrata desde registry.

### B5.2 - Canal MQTT admin (set/get) para config runtime

- Archivos a tocar:
  - `Firmware/port/src/app/config/app_config_mqtt_admin.cpp`
  - `Firmware/port/include/app/config/app_config_mqtt_admin.h`
  - `Firmware/port/src/app/comms/comms_legacy_backend.cpp`
  - `Firmware/port/src/app/comms/comms_ha_backend.cpp`
- Keys afectadas:
  - Reusa `1000..1015` (sin duplicar mapa)
- Criterios de aceptación (sin placa):
  - Parseo robusto de `get` y JSON parcial.
  - ACK consistente con `ok/reboot_required/error`.
  - Simulación de mensajes (`app_mqtt_smoketest`) con trazas de topics esperados.

### B5.3 - commit/CRC/rollback (si aplica)

- Archivos a tocar:
  - `Firmware/port/src/app/nvm/eeprom_registry.cpp`
  - `Firmware/port/include/app/nvm/eeprom_registry.h`
  - `Firmware/port/src/app/config/app_config.cpp`
- Enfoque:
  - Doble fase lógica para config crítica: write staging + commit flag/version.
  - Si CRC falla en boot: fallback a último snapshot válido o defaults.
- Keys sugeridas auxiliares:
  - `1090` cfg_generation (u32)
  - `1091` cfg_last_good_generation (u32)
- Criterios de aceptación (sin placa):
  - Corrupción simulada de payload => arranque no bloquea y recupera defaults/último válido.
  - Test de idempotencia: múltiples `set` consecutivos mantienen lectura consistente.

## D) Backlog B6 (portado de módulos del original)

### 1) Sensores básicos (DS18B20 agua/aire)

- Fuente en Original (lectura):
  - `Firmware/Original/Ferduino_Aquarium_Controller/src/Modules/Parametros.h`
  - `Firmware/Original/Ferduino_Aquarium_Controller/src/Modules/Stamps_V4.h`
  - `Firmware/Original/Ferduino_Aquarium_Controller/src/Modules/Stamps_EZO.h`
  - `Firmware/Original/Bibliotecas/OneWire/OneWire.h`
  - `Firmware/Original/Bibliotecas/Arduino-Temperature-Control-Library/DallasTemperature.h`
- API objetivo en port:
  - Nueva capa `app/sensors` con interfaz `readTemperatures()` y estado normalizado.
- Dependencias:
  - HAL GPIO/I2C (según sensor), NVM (calibración), comms legacy/HA.
- Smoke test sin placa:
  - Backend sensor fake con valores deterministas y publicación a legacy ID0 + HA state.
- Riesgos y mitigaciones:
  - Bloqueos de bus OneWire -> timeout estricto.
  - RAM limitada -> buffers estáticos, sin `String`.

### 2) Actuadores simples (relés/salidas)

- Fuente en Original:
  - `Firmware/Original/Ferduino_Aquarium_Controller/src/Modules/Parametros.h`
  - `Firmware/Original/Ferduino_Aquarium_Controller/src/Modules/Loop.h`
- API objetivo en port:
  - `app/actuators/outlets` sobre `hal_relay` + mapping fijo outlet->relay.
- Dependencias:
  - HAL relay/ioexpander, comms HA (`cmd/outlet`) y legacy (ID0 + comandos write).
- Smoke test sin placa:
  - Simular toggle outlets y validar reflejo en payload HA/legacy.
- Riesgos y mitigaciones:
  - Polaridad ACTIVE_LOW confusa -> tabla de verdad y test unitario simple por pin lógico/físico.

### 3) PWM/LED básico (setpoints)

- Fuente en Original:
  - `Firmware/Original/Ferduino_Aquarium_Controller/src/Modules/Leds.h`
- API objetivo en port:
  - `app/lighting` con setpoint por canal y apply por `hal_pwm`.
- Dependencias:
  - HAL PWM, NVM (setpoints), comms ID4/ID5 + HA sensors.
- Smoke test sin placa:
  - Inyección de setpoints y verificación de clamping 0..255 + payload publicado.
- Riesgos y mitigaciones:
  - Timings PWM/ciclo loop -> separar cálculo de scheduler vs salida física.

### 4) RTC/timekeeping

- Fuente en Original:
  - `Firmware/Original/Ferduino_Aquarium_Controller/src/Modules/Temporizadores.h`
  - `Firmware/Original/Ferduino_Aquarium_Controller/src/Modules/Parametros.h`
- API objetivo en port:
  - `app/time_service` sobre `hal_rtc` con funciones `now()`, `set()`, `isValid()`.
- Dependencias:
  - HAL RTC, comms legacy ID2, NVM para TZ/políticas si aplica.
- Smoke test sin placa:
  - RTC fake con fecha fija y comandos ID2 read/write simulados.
- Riesgos y mitigaciones:
  - Deriva/hora inválida -> fallback claro a uptime + flag de calidad de tiempo.

### 5) Dosing schedules

- Fuente en Original:
  - `Firmware/Original/Ferduino_Aquarium_Controller/src/Modules/Dosadoras.h`
  - `Firmware/Original/Ferduino_Aquarium_Controller/src/Modules/Funcoes_EEPROM.h`
- API objetivo en port:
  - `app/dosing` con structs compactos de horario, dosis, días y estado.
- Dependencias:
  - NVM registry, hal_time/hal_rtc, salidas de actuadores.
- Smoke test sin placa:
  - Simulación reloj acelerado + ejecución de ventanas de dosis en memoria.
- Riesgos y mitigaciones:
  - Complejidad de calendario -> validadores estrictos de rango y pruebas por tabla de casos.

### 6) UI original (UTFT/URTouch) - posponer o aislar

- Fuente en Original:
  - `Firmware/Original/Ferduino_Aquarium_Controller/src/Modules/Menus.h`
  - `Firmware/Original/Ferduino_Aquarium_Controller/src/Modules/ProcessMyTouch.h`
  - `Firmware/Original/Bibliotecas/UTFT/*`
  - `Firmware/Original/Bibliotecas/URTouch/*`
- API objetivo en port:
  - `app/ui` desacoplada (adaptador opcional), no core del control.
- Dependencias:
  - Muy altas (display/touch/fonts/memoria).
- Smoke test sin placa:
  - Ninguno prioritario; mantener fuera de `mega2560_port` inicial.
- Riesgos y mitigaciones:
  - Alto consumo RAM/flash -> feature flag `FEAT_UI_UTFT=0` por defecto.

## E) Compatibilidad legacy + HA (convivencia)

### Reparto funcional recomendado

- Legacy MQTT:
  - Mantener protocolo de control/compatibilidad externa (IDs/topics actuales).
- HA:
  - Operación moderna (discovery + entidades + control outlets + telemetría).

### Cómo evitar duplicación de lógica

- Single source of truth:
  - Crear modelo interno único de estado y comandos en `app/model`.
- Adaptadores:
  - Legacy backend serializa/deserializa modelo a JSON legacy.
  - HA backend serializa/deserializa modelo a topics/state HA.
- Regla:
  - Ningún backend toca HAL directamente salvo para I/O de transporte MQTT.

### Modelo interno mínimo propuesto

```cpp
struct SystemState {
  uint32_t uptime_s;
  float twater_c;
  float theatsink_c;
  float tamb_c;
  float water_ph;
  float reactor_ph;
  float orp_mv;
  float salinity;
  uint8_t led_power[5];      // white, blue, royal, red, uv
  uint8_t outlet[9];         // 0/1
};

struct SystemConfig {
  uint8_t backend_mode;      // legacy/ha
  char mqtt_host[64];
  uint16_t mqtt_port;
  char device_id[48];
  bool use_dhcp;
  uint8_t mac[6];
  uint8_t ip[4], gw[4], mask[4], dns[4];
};
```

## F) Persistencia: EEPROM / FRAM / logger

### Separación por responsabilidad

- Config:
  - Hoy: EEPROM (registry TLV).
  - Futuro: FRAM pequeña dedicada (rápida, alta durabilidad).
- Log:
  - Futuro: FRAM grande como ring-buffer de telemetría/eventos.
  - SD como archivo de archivo (flush por lotes).

### Propuesta de hardware de persistencia

- FRAM pequeña config:
  - 32KB a 128KB (I2C o SPI) suficiente para varias generaciones de config + metadatos.
- FRAM grande logger:
  - 1MB recomendado base (256KB mínimo; 4MB si se quiere histórico amplio).

### Política de flush a SD (anti desgaste/corrupción)

- Ring-buffer en FRAM como journal transaccional.
- Flush por batch a SD cada N muestras o cada T minutos (lo que ocurra primero).
- Marcar checkpoints (`last_flushed_seq`) para recuperación tras reboot.
- Nunca bloquear loop de control esperando I/O SD.

### Formato de log recomendado

- Recomendado en AVR: binario compacto con header + CRC por registro.
  - Menor CPU/RAM que JSON.
  - Parseable offline por script.
- Alternativas:
  - CBOR viable si hay tooling, pero más complejidad.
  - JSON no recomendado como formato primario en AVR.

### Estimación de memoria y retención

Suposiciones:
- Registro binario de 32 bytes (timestamp + sensores + estados + crc)

1) Muestreo cada 1 minuto
- 1 día: 1,440 registros -> ~46 KB
- 7 días: 10,080 registros -> ~322 KB
- 30 días: 43,200 registros -> ~1.38 MB

2) Muestreo cada 5 segundos
- 1 día: 17,280 registros -> ~553 KB
- 7 días: 120,960 registros -> ~3.87 MB
- 30 días: 518,400 registros -> ~16.6 MB

Recomendación práctica:
- 256KB: solo ventana corta (horas a pocos días en 1/min).
- 1MB: buen equilibrio inicial para 1/min (~22 días aprox con 32B/registro).
- 4MB: recomendado si se quiere >7 días en 5s o >80 días en 1/min.

## G) Plan de commits (B5 + primeros 2 módulos B6)

### Secuencia propuesta (commits cortos)

1. `docs(port): add B5/B6 executable porting plan`
- Archivos:
  - `Firmware/port/docs/PORTING_PLAN_B5_B6.md`
- Criterio:
  - Documentación trazable en repo.

2. `nvm(config): define registry keys for runtime mqtt/network`
- Archivos:
  - `Firmware/port/include/app/nvm/registry_keys.h` (nuevo)
  - `Firmware/port/include/app/nvm/eeprom_registry.h`
  - `Firmware/port/src/app/nvm/eeprom_migration.cpp`
- Criterio `compila + smoke`:
  - `pio run -d Firmware/port -e mega2560_port`

3. `config: load/save runtime config via registry with build-flag fallback`
- Archivos:
  - `Firmware/port/src/app/config/app_config.cpp`
  - `Firmware/port/include/app/config/app_config.h`
- Criterio:
  - build verde + lectura defaults con registry vacío.

4. `cfgadmin: persist mqtt/network changes into registry`
- Archivos:
  - `Firmware/port/src/app/config/app_config_mqtt_admin.cpp`
  - `Firmware/port/include/app/config/app_config_mqtt_admin.h`
  - `Firmware/port/src/app/comms/comms_legacy_backend.cpp`
  - `Firmware/port/src/app/comms/comms_ha_backend.cpp`
- Criterio:
  - build verde + mensajes cfg GET/SET coherentes en logs smoke.

5. `model: introduce internal system state for legacy/ha adapters`
- Archivos:
  - `Firmware/port/include/app/model/system_state.h` (nuevo)
  - `Firmware/port/src/app/model/system_state.cpp` (nuevo)
  - adaptaciones mínimas en comms
- Criterio:
  - build verde + legacy y HA publican desde mismo estado base.

6. `b6(sensor): add ds18b20 service with fake provider + smoke path`
- Archivos:
  - `Firmware/port/include/app/sensors/ds18b20_service.h` (nuevo)
  - `Firmware/port/src/app/sensors/ds18b20_service.cpp` (nuevo)
  - integración en payload ID0/HA state
- Criterio:
  - build verde + smoke sin placa publica temperaturas fake.

7. `b6(actuator): wire outlet control to relay mapping + state reporting`
- Archivos:
  - `Firmware/port/src/app/comms/ha/ha_outlet_control.cpp`
  - `Firmware/port/src/hal/impl/mega2560/hal_relay.cpp`
  - adaptadores legacy/HA para reflejar estado real
- Criterio:
  - build verde + smoke de toggle outlets refleja cambios en ambos backends.

## Prioridad operativa recomendada

Primero implementar **B5.1** antes de módulos B6.

Razón:
- Define el contrato de configuración persistente (source of truth de runtime).
- Evita portar módulos sobre una base de config temporal (EEPROM offset 0) que luego habría que rehacer.
- Reduce retrabajo en B6 porque sensores/actuadores necesitarán inmediatamente config estable de red/backend/device.
