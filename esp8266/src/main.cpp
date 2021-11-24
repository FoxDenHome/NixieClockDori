#include <ESP8266WiFi.h>
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <time.h>
#include <coredecls.h>
#include <sntp.h>
#include <PubSubClient.h>

#include "config.h"
#include "serial_arduino.h"
#include "eeprom.h"
#include "mqtt.h"

uint32_t sntp_update_delay_MS_rfc_not_less_than_15000() {
  return NTP_UPDATE_INTERVAL;
}

bool wifiConnected = false;

time_t now;
tm now_tm;
char timeSerialBuffer[32];
void time_is_set(bool is_sntp){
  if (!is_sntp) {
    return;
  }
  time(&now);
  localtime_r(&now, &now_tm);

  sprintf(timeSerialBuffer, "T%02d%02d%02d%02d%02d%02d", now_tm.tm_hour, now_tm.tm_min, now_tm.tm_sec, now_tm.tm_mday, now_tm.tm_mon + 1, now_tm.tm_year % 100);
  arduinoSerial.send(String(timeSerialBuffer));
}

char tz[EEPROM_LEN_STRING];
char ntp_server[EEPROM_LEN_STRING];
char hostname[EEPROM_LEN_STRING];
void setup() {
  arduinoSerial.init();
  arduinoSerial.echo(F("Booting..."));
  eepromInit();

  WiFi.mode(WIFI_STA);

  strcpy(hostname, eepromRead(EEPROM_OTA_HOSTNAME).c_str());
  if (strlen(hostname) < 1) {
    strcpy(hostname, WiFi.getHostname());
  }

  WiFi.setHostname(hostname);
  WiFi.begin(eepromRead(EEPROM_WIFI_SSID), eepromRead(EEPROM_WIFI_PASSWORD));

  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    arduinoSerial.echo(F("Connection Failed! Please configure!"));
  } else {
    arduinoSerial.echo(F("Connected"));
  }

  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.setPassword(eepromRead(EEPROM_OTA_PASSWORD).c_str());
  ArduinoOTA.begin();

  settimeofday_cb(time_is_set);

  strcpy(tz, eepromRead(EEPROM_TIME_ZONE).c_str());
  strcpy(ntp_server, eepromRead(EEPROM_NTP_SERVER).c_str());
  configTime(tz, ntp_server);

  mqttInit();
}

void checkWiFiTimeout() {
  static unsigned long lastWiFiConnected = 0;
  static unsigned long lastWiFiChecked = 0;
  const unsigned long currentMilis = millis();

  if ((currentMilis - lastWiFiChecked) <= WIFI_CHECK_INTERVAL) {
    return;
  }
  lastWiFiChecked = currentMilis;

  if (WiFi.isConnected()) {
    lastWiFiConnected = currentMilis;
    return;
  }

  if ((currentMilis - lastWiFiConnected) <= WIFI_TIMEOUT) {
    return;
  }

  arduinoSerial.echo(F("Not connected for too long. Resetting..."));
  ESP.reset();
}

void loop() {
  arduinoSerial.loop();
  ArduinoOTA.handle();
  mqttLoop();
  checkWiFiTimeout();
}
