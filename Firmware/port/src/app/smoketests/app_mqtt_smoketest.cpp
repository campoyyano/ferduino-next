#include <Arduino.h>

#include "hal/hal_network.h"
#include "hal/hal_mqtt.h"

#ifndef MQTT_BROKER_HOST
  #define MQTT_BROKER_HOST "192.168.1.10"
#endif

#ifndef MQTT_BROKER_PORT
  #define MQTT_BROKER_PORT 1883
#endif

static void onMqttMessage(const char* topic, const uint8_t* payload, size_t len) {
  Serial.print("RX topic=");
  Serial.print(topic);
  Serial.print(" len=");
  Serial.println((unsigned)len);

  Serial.print("payload: ");
  for (size_t i = 0; i < len; ++i) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void app_mqtt_smoketest() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("=== A9.3 MQTT smoketest ===");

  // 1) Network
  hal::NetworkConfig ncfg;
  ncfg.mac[0] = 0x02; ncfg.mac[1] = 0xF0; ncfg.mac[2] = 0xED;
  ncfg.mac[3] = 0x00; ncfg.mac[4] = 0x00; ncfg.mac[5] = 0x02;
  ncfg.useDhcp = true;

  const auto nerr = hal::network().begin(ncfg);
  if (nerr != hal::NetError::Ok) {
    Serial.print("ERROR: network begin failed, err=");
    Serial.println((int)nerr);
    for (;;) delay(1000);
  }
  Serial.println("Network OK");

  // 2) MQTT
  hal::MqttConfig mcfg;
  mcfg.host = MQTT_BROKER_HOST;
  mcfg.port = MQTT_BROKER_PORT;
  mcfg.clientId = "ferduino-next-mega";
  mcfg.keepAliveSec = 30;

  const auto merr0 = hal::mqtt().begin(mcfg);
  if (merr0 != hal::MqttError::Ok) {
    Serial.print("ERROR: mqtt begin failed, err=");
    Serial.println((int)merr0);
    for (;;) delay(1000);
  }

  hal::mqtt().onMessage(onMqttMessage);

  Serial.println("Connecting MQTT...");
  const auto merr1 = hal::mqtt().connect();
  if (merr1 != hal::MqttError::Ok) {
    Serial.print("ERROR: mqtt connect failed, err=");
    Serial.println((int)merr1);
    for (;;) delay(1000);
  }
  Serial.println("MQTT connected");

  (void)hal::mqtt().subscribe("ferduino-next/test");

  const char msg[] = "hello from mega2560";
  (void)hal::mqtt().publish("ferduino-next/test", (const uint8_t*)msg, sizeof(msg) - 1, false);

  for (;;) {
    hal::network().loop();
    hal::mqtt().loop();

    if (!hal::mqtt().connected()) {
      Serial.println("MQTT disconnected, reconnecting...");
      (void)hal::mqtt().connect();
    }

    delay(10);
  }
}
