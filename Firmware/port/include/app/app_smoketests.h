#pragma once

// IDs numéricos para seleccionar con build_flags:
//  -D SMOKETEST=SMOKETEST_NONE  (runtime normal)
//  -D SMOKETEST=SMOKETEST_GPIO
//  -D SMOKETEST=SMOKETEST_MQTT
// etc.
#define SMOKETEST_NONE          0
#define SMOKETEST_GPIO          1
#define SMOKETEST_FERDUINO_PINS 2
#define SMOKETEST_PWM           3
#define SMOKETEST_IOX           4
#define SMOKETEST_RTC           5
#define SMOKETEST_RELAY         6
#define SMOKETEST_SERIAL        7
#define SMOKETEST_NETWORK       8
#define SMOKETEST_MQTT          9

void app_gpio_smoketest_run();
void app_ferduino_pins_smoketest_run();
void app_pwm_leds_smoketest_run();
void app_pcf8575_smoketest_run();
void app_ioexpander_smoketest_run();
void app_serial_smoketest_run();
void app_rtc_smoketest_run();
void app_relay_smoketest_run();
void app_network_smoketest();
void app_mqtt_smoketest();
