#include <Arduino.h>

#include "pins/pins_profiles.h"
#include "hal/hal_gpio.h"
#include "hal/hal_time.h"
#include "hal/hal_log.h"
#include "app/app_smoketests.h"


void setup() {
  Serial.begin(115200);

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


#if (SMOKETEST == SMOKETEST_GPIO)
  // A2: test mínimo (LED D13)
  app_gpio_smoketest_run();

#elif (SMOKETEST == SMOKETEST_FERDUINO_PINS)
  // A3: test con pines reales Ferduino
  app_ferduino_pins_smoketest_run();

#elif (SMOKETEST == SMOKETEST_PWM)
  // A4: test con pwm pines reales arduino
  app_pwm_leds_smoketest_run();

#elif (SMOKETEST == SMOKETEST_IOX)
  //A5: Test del PCF8575
  app_ioexpander_smoketest_run();

#elif (SMOKETEST == SMOKETEST_RTC)
   // A7: Test RTC
  app_rtc_smoketest_run();

#elif (SMOKETEST == SMOKETEST_RELAY)
  // A8: PCF8575 Test_2;
  app_relay_smoketest_run();

#elif (SMOKETEST == SMOKETEST_SERIAL)
  // A6: Test Uart/Debug
  app_serial_smoketest_run();

#else
  #error "SMOKETEST invalido. Revisa smoketest_select.h"
#endif

}


