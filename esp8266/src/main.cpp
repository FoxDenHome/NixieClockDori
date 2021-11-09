#include <ESP8266WiFi.h>
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <time.h>
#include <coredecls.h>
#include <sntp.h>

#include "config.h"
#include "serial.h"
#include "eeprom.h"

uint32_t sntp_update_delay_MS_rfc_not_less_than_15000() {
  return NTP_UPDATE_INTERVAL;
}

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

char tz[EEPROM_LEN_STRING];
char ntp_server[EEPROM_LEN_STRING];
char hostname[EEPROM_LEN_STRING];
void setup() {
  Serial.begin(115200);
  Serial.println(F("^EBooting...$"));
  eepromInit();

  WiFi.mode(WIFI_STA);
  WiFi.begin(eepromRead(EEPROM_WIFI_SSID), eepromRead(EEPROM_WIFI_PASSWORD));

  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println(F("^EConnection Failed! Please configure!$"));
  } else {
    Serial.println(F("^EConnected!$"));
  }

  strcpy(hostname, eepromRead(EEPROM_OTA_HOSTNAME).c_str());
  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.setPassword(eepromRead(EEPROM_OTA_PASSWORD).c_str());
  ArduinoOTA.begin();

  settimeofday_cb(time_is_set);

  strcpy(tz, eepromRead(EEPROM_TIME_ZONE).c_str());
  strcpy(ntp_server, eepromRead(EEPROM_NTP_SERVER).c_str());
  configTime(tz, ntp_server);
}

void loop() {
  ArduinoOTA.handle();
  serialLoop();
}
