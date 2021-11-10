#include <Arduino.h>
#include <ESP8266httpUpdate.h>

#include "serial_arduino.h"
#include "eeprom.h"

ArduinoSerial::ArduinoSerial(HardwareSerial& serial) : CommandSerial(serial) {

}

void ArduinoSerial::processEEPROMCommand(const int offset) {
    if (this->buffer.length() < 1) {
        this->replyFirst("Read: ");
        this->sendEnd(eepromRead(offset));
        return;
    }
    eepromWrite(offset, this->buffer);
    this->reply("Write OK");
}

void ArduinoSerial::httpFlash() {
        WiFiClient client;
        ESPhttpUpdate.rebootOnUpdate(false);
        HTTPUpdateResult ret = ESPhttpUpdate.update(client, this->buffer);
        switch (ret) {
            case HTTP_UPDATE_FAILED:
                this->replyFirst("FAIL");
                this->sendEnd(ESPhttpUpdate.getLastErrorString());
                break;
            case HTTP_UPDATE_NO_UPDATES:
                this->reply("No updates");
                break;
            case HTTP_UPDATE_OK:
                this->reply("Update OK");
                ESP.restart();
                break;
            default:
                this->reply("Unknown update error");
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
        case 'C': // Commit
            eepromCommit();
            this->reply("Commit OK");
            break;
        case 'R': // Reset
            this->reply("Reset");
            ESP.reset();
            break;
        case 'F': // Flash from HTTP
            this->httpFlash();
            break;
        default:
            this->reply("Unknown command");
            break;
    }
}
