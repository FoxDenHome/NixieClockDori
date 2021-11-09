#include <ESP8266WiFi.h>
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <time.h>
#include <coredecls.h>

#include "config.h"

time_t now;
tm now_tm;
void time_is_set(bool is_sntp){
  if (!is_sntp) {
    return;
  }
  time(&now);
  localtime_r(&now, &now_tm);
  Serial.printf("^T%02d%02d%02d%02d%02d%02d$\n", now_tm.tm_hour, now_tm.tm_min, now_tm.tm_sec, now_tm.tm_mday, now_tm.tm_mon, now_tm.tm_year % 100);
}

void setup() {
  Serial.begin(115200);
  Serial.println("^EBooting...$");
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

  settimeofday_cb(time_is_set);
  configTime(TIME_ZONE, NTP_SERVER);
}

void loop() {
  ArduinoOTA.handle();
}
