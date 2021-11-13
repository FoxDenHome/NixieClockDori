#include <Arduino.h>
#include <ESP8266httpUpdate.h>

#include "serial_arduino.h"
#include "eeprom.h"
#include "mqtt.h"

ArduinoSerial arduinoSerial(Serial);

ArduinoSerial::ArduinoSerial(HardwareSerial& serial) : CommandSerial(serial) {

}

void ArduinoSerial::processEEPROMCommand(const int offset) {
    if (this->buffer.length() < 1) {
        this->replyFirst(F("OK Read: "));
        this->sendEnd(eepromRead(offset));
        return;
    }
    eepromWrite(offset, this->buffer);
    this->reply(F("OK Write"));
}

void ArduinoSerial::httpFlash() {
        WiFiClient client;
        ESPhttpUpdate.rebootOnUpdate(false);
        HTTPUpdateResult ret = ESPhttpUpdate.update(client, this->buffer);
        switch (ret) {
            case HTTP_UPDATE_FAILED:
                this->replyFirst(F("BAD "));
                this->sendEnd(ESPhttpUpdate.getLastErrorString());
                break;
            case HTTP_UPDATE_NO_UPDATES:
                this->reply(F("OK No updates"));
                break;
            case HTTP_UPDATE_OK:
                this->reply(F("OK Updated"));
                ESP.restart();
                break;
            default:
                this->reply(F("BAD Unknown update error"));
                break;
        }
}

void ArduinoSerial::handle() {
    switch (this->command) {
        case 'S': // SSID
            this->processEEPROMCommand(EEPROM_WIFI_SSID);
            break;
        case 'P': // Password
            this->processEEPROMCommand(EEPROM_WIFI_PASSWORD);
            break;
        case 'N': // NTP
            this->processEEPROMCommand(EEPROM_NTP_SERVER);
            break;
        case 'T': // TimeZone
            this->processEEPROMCommand(EEPROM_TIME_ZONE);
            break;
        case 'H': // Hostname
            this->processEEPROMCommand(EEPROM_OTA_HOSTNAME);
            break;
        case 'O': // OTA password
            this->processEEPROMCommand(EEPROM_OTA_PASSWORD);
            break;
        case 'M': // MQTT broker/server
            this->processEEPROMCommand(EEPROM_MQTT_BROKER);
            break;
        case 'A': // MQTT username
            this->processEEPROMCommand(EEPROM_MQTT_USERNAME);
            break;
        case 'B': // MQTT password
            this->processEEPROMCommand(EEPROM_MQTT_PASSWORD);
            break;
        case 'D': // MQTT topic
            this->processEEPROMCommand(EEPROM_MQTT_TOPIC);
            break;
        case 'C': // Commit
            eepromCommit();
            this->reply(F("OK Commit"));
            break;
        case 'R': // Reset
            this->reply(F("OK Reset"));
            this->serial->print("\n\n\n\n");
            ESP.reset();
            break;
        case 'F': // Flash from HTTP
            this->httpFlash();
            break;
        case '<': // Echo, ignore
            mqttSend(this->command + this->buffer);
            break;
        case '$': // Command reply, ignore
            mqttSend(this->command + this->buffer);
            break;
        default:
            this->reply(F("BAD Unknown command"));
            break;
    }
}
