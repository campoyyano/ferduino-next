#include <Arduino.h>

#include "pins/pins_profiles.h"
#include "app/app_smoketests.h"

#include "app/runtime/app_runtime.h"

void setup() {
  Serial.begin(115200);

  Serial.println("ferduino-next: boot");
  Serial.print("Perfil de pines: ");
  Serial.println(PINS_PROFILE);

  // En runtime, begin() se llama desde loop si procede.
}

void loop() {
#if (SMOKETEST == SMOKETEST_NONE)
  app::runtime::loop();

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
  app::runtime::loop();
#endif
}
