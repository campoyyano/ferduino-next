#include <Arduino.h>

#include "pins_profiles.h"
#include "hal_gpio.h"
#include "hal_time.h"
#include "hal_log.h"
#include "app_smoketests.h"


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

  // A2: test mínimo (LED D13)
  // app_gpio_smoketest_run();

  // A3: test con pines reales Ferduino
  // app_ferduino_pins_smoketest_run();

  // A4: test con pwm pines reales arduino
  // app_pwm_leds_smoketest_run(); 

  //A5: Test del PCF8575
  // app_pcf8575_smoketest_run();
    app_ioexpander_smoketest_run();

  // A6: Test Uart/Debug
    app_serial_smoketest_run();

  // A7: Test RTC
    app_rtc_smoketest_run();


}
