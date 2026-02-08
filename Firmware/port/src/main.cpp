#include <Arduino.h>

#include "pins/pins_profiles.h"
#include "hal/hal_gpio.h"
#include "hal/hal_time.h"
#include "hal/hal_log.h"
#include "app/app_smoketests.h"

#include "app/nvm/eeprom_registry.h"
#include "app/nvm/eeprom_migration.h"
#include "app/config/app_config.h"

void setup() {
  Serial.begin(115200);

  // NVM: inicializa registry TLV (>=1024) y migra subset legacy si aplica.
  // Importante: no escribir jamás en 0..1023 desde el port.
  (void)app::nvm::registry().begin();
  (void)app::nvm::migrateLegacyIfNeeded();
  (void)app::cfg::loadOrDefault();

  pinMode(alarmPin, OUTPUT);
  pinMode(desativarFanPin, OUTPUT);

  Serial.println("ferduino-next: port skeleton");
  Serial.print("Perfil de pines: ");
  Serial.println(PINS_PROFILE);
}

void loop() {
  // Parpadeo simple para validar que compila y usa los pines del perfil
  digitalWrite(alarmPin, HIGH);
  delay(200);
  digitalWrite(alarmPin, LOW);
  delay(800);

#if (SMOKETEST == SMOKETEST_NONE)
  // Sin smoketest: dejar solo el loop base (parpadeo) o futura app runtime.

#elif (SMOKETEST == SMOKETEST_GPIO)
  app_gpio_smoketest_run();

#elif (SMOKETEST == SMOKETEST_FERDUINO_PINS)
  app_ferduino_pins_smoketest_run();

#elif (SMOKETEST == SMOKETEST_PWM)
  app_pwm_leds_smoketest_run();

#elif (SMOKETEST == SMOKETEST_IOX)
  app_ioexpander_smoketest_run();

#elif (SMOKETEST == SMOKETEST_RTC)
  app_rtc_smoketest_run();

#elif (SMOKETEST == SMOKETEST_RELAY)
  app_relay_smoketest_run();

#elif (SMOKETEST == SMOKETEST_SERIAL)
  app_serial_smoketest_run();

#elif (SMOKETEST == SMOKETEST_NETWORK)
  app_network_smoketest();

#elif (SMOKETEST == SMOKETEST_MQTT)
  app_mqtt_smoketest();

#else
  #error "SMOKETEST value not supported. Define SMOKETEST=SMOKETEST_NONE|GPIO|FERDUINO_PINS|PWM|IOX|RTC|RELAY|SERIAL|NETWORK|MQTT"
#endif
}
