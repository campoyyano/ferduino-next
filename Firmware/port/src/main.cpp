#include <Arduino.h>

#include "pins_profiles.h"
#include "hal_gpio.h"
#include "hal_time.h"
#include "hal_log.h"

void app_gpio_smoketest_run();

void setup() {
  Serial.begin(115200);

  pinMode(alarmPin, OUTPUT);
  pinMode(desativarFanPin, OUTPUT);

  Serial.println("ferduino-next: port skeleton");
  Serial.print("Perfil de pines: ");
  Serial.println(PINS_PROFILE);
  app_gpio_smoketest_run();

}

void loop() {
  // Parpadeo simple para validar que compila y usa los pines del perfil
  digitalWrite(alarmPin, HIGH);
  delay(200);
  digitalWrite(alarmPin, LOW);
  delay(800);
}
