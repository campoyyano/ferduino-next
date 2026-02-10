#include "app/temp_control/temp_control.h"

#include <Arduino.h>

#include "app/app_build_flags.h"
#include "app/nvm/eeprom_registry.h"
#include "app/sensors/sensors.h"

#include "hal/hal_gpio.h"
#include "pins/pins_ferduino_mega2560.h"

namespace app::tempctrl {

// TLV keys (ya documentadas y migradas desde legacy)
static constexpr uint16_t kKeySetX10   = 100;
static constexpr uint16_t kKeyOffX10   = 101;
static constexpr uint16_t kKeyAlarmX10 = 102;

// Límites de seguridad (mismo espíritu que el original)
static constexpr int16_t kMinSafeC_x10 = 100;  // 10.0°C
static constexpr int16_t kMaxSafeC_x10 = 500;  // 50.0°C

// Periodicidad de control (ms)
static constexpr uint32_t kControlPeriodMs = 1000;

static State g_state{};
static uint32_t g_lastRunMs = 0;

static int16_t clamp_i16(int32_t v, int16_t lo, int16_t hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return (int16_t)v;
}

static int16_t c_to_x10(float c) {
  // Redondeo simple a décimas: c>=0 en sensores típicos
  const float x = c * 10.0f;
  const int16_t r = (int16_t)(x + 0.5f);
  return r;
}

static Config loadCfg() {
  Config c{};

  // Defaults: consistentes con el original
  c.set_x10   = 255; // 25.5°C
  c.off_x10   = 5;   // 0.5°C (banda muerta)
  c.alarm_x10 = 10;  // 1.0°C

  auto& reg = app::nvm::registry();
  int32_t v = 0;

  if (reg.getI32(kKeySetX10, v)) {
    c.set_x10 = clamp_i16(v, 100, 400); // 10.0..40.0°C
  }
  if (reg.getI32(kKeyOffX10, v)) {
    c.off_x10 = clamp_i16(v, 1, 300);   // 0.1..30.0°C
  }
  if (reg.getI32(kKeyAlarmX10, v)) {
    c.alarm_x10 = clamp_i16(v, 0, 300); // 0.0..30.0°C
  }

  return c;
}

static void applyOutputs(bool heaterOn, bool chillerOn) {
#if APP_TEMPCTRL_USE_GPIO
  // En el original estas salidas son pines directos Mega, no PCF8575.
  hal::escribirPin(pins_base::aquecedorPin, heaterOn);
  hal::escribirPin(pins_base::chillerPin, chillerOn);
#else
  (void)heaterOn;
  (void)chillerOn;
#endif
}

void begin() {
  g_state = State{};
  g_lastRunMs = 0;

  g_state.cfg = loadCfg();

#if APP_TEMPCTRL_USE_GPIO
  hal::modoPin(pins_base::aquecedorPin, hal::PinMode::Salida);
  hal::modoPin(pins_base::chillerPin, hal::PinMode::Salida);
  applyOutputs(false, false);
#endif
}

void loop() {
  const uint32_t now = millis();
  if (now - g_lastRunMs < kControlPeriodMs) return;
  g_lastRunMs = now;

  // Releer cfg de NVM (permite cambios vía MQTT admin / registry).
  g_state.cfg = loadCfg();

  const auto temps = app::sensors::last();

  // Para el control térmico solo nos importa la sonda de agua.
  if (!temps.water_valid) {
    g_state.valid = false;
    g_state.alarm_active = false;
    g_state.heater_on = false;
    g_state.chiller_on = false;
    g_state.water_x10 = 0;
    applyOutputs(false, false);
    return;
  }

  g_state.valid = true;
  g_state.water_x10 = c_to_x10(temps.water_c);

  const int16_t set   = g_state.cfg.set_x10;
  const int16_t off   = g_state.cfg.off_x10;
  const int16_t alarm = g_state.cfg.alarm_x10;
  const int16_t t     = g_state.water_x10;

  // Alarma si fuera de (set ± off ± alarm)
  if (alarm > 0) {
    const int16_t hi = (int16_t)(set + off + alarm);
    const int16_t lo = (int16_t)(set - off - alarm);
    g_state.alarm_active = (t >= hi) || (t <= lo);
  } else {
    g_state.alarm_active = false;
  }

  // Control principal: banda muerta set ± off
  bool heater = g_state.heater_on;
  bool chiller = g_state.chiller_on;

  // Dentro de banda muerta => ambos OFF
  if ((t < (int16_t)(set + off)) && (t > (int16_t)(set - off))) {
    heater = false;
    chiller = false;
  } else {
    // Fuera de banda -> actuar
    if (off > 0) {
      if (t > (int16_t)(set + off)) {
        chiller = true;
        heater = false;
      }
      if (t < (int16_t)(set - off)) {
        heater = true;
        chiller = false;
      }
    } else {
      heater = false;
      chiller = false;
    }
  }

  // Safety hard-cut
  if (t > kMaxSafeC_x10 || t < kMinSafeC_x10) {
    heater = false;
    chiller = false;
  }

  // Evitar ambos ON simultáneamente
  if (heater && chiller) {
    heater = false;
    chiller = false;
  }

  g_state.heater_on = heater;
  g_state.chiller_on = chiller;

  applyOutputs(heater, chiller);
}

State last() {
  return g_state;
}

} // namespace app::tempctrl
