#include "Config.h"
#include "WIFI.h"
#include "Server.h"
#include "MQTT.h"

unsigned long lastMsg = 0;

void setup() {
  pinMode(led, OUTPUT);
  Serial.begin(115200);
  init_WIFI(WIFI_START_MODE_CLIENT);
  init_server();
  init_MQTT();
  mqtt_cli.subscribe("esp8266/command");
}

void loop() {
  server.handleClient();
  mqtt_cli.loop();
  
  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    int data = analogRead(A0);
    mqtt_cli.publish("esp8266/sensor", String(data).c_str());
  }
}