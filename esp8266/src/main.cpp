#include <ESP8266WiFi.h>
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <NTPClient.h>

#include "config.h"

const long utcOffsetInSeconds = 3600 * TIME_ZONE;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "192.168.4.10", utcOffsetInSeconds);

unsigned long lastRun = LONG_MAX;

void setup() {
  Serial.begin(115200);
  Serial.println("^E Booting...$");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("^EConnection Failed! Rebooting...$");
    delay(5000);
    ESP.restart();
  }
  Serial.println("^EConnected!$");

  ArduinoOTA.setHostname("esp-nixie-clock-dori");
  ArduinoOTA.setPassword((const char *)OTA_PASSWORD);
  ArduinoOTA.begin();

  timeClient.begin();
}

void loop() {
	const unsigned long curMillis = millis();
	const unsigned long timeSinceLast = curMillis - lastRun;

  ArduinoOTA.handle();

  if (timeSinceLast >= 60000UL) {
    timeClient.forceUpdate();
    const unsigned long time = timeClient.getEpochTime();
    if (time > 0) {
      Serial.print("^T");
      Serial.print(time);
      Serial.println("$");
    }
    lastRun = curMillis;
  }
}
