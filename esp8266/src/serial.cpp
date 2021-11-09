#include <Arduino.h>
#include <ESP8266httpUpdate.h>

#include "config.h"
#include "eeprom.h"

#define STATE_LOOKING_FOR_START 0
#define STATE_LOOKING_FOR_COMMAND 1
#define STATE_LOOKING_FOR_END 2

byte serialCommandState;
char serialCommand;
String inputString;

void serialInit() {
    Serial.begin(115200);
    inputString.reserve(128);
    inputString = "";
    serialCommandState = STATE_LOOKING_FOR_START;
}

static void processEEPROMCommand(const int offset) {
    if (inputString.length() < 1) {
        Serial.print("^RRead: ");
        Serial.print(eepromRead(offset));
        Serial.println("$");
        return;
    }
    eepromWrite(offset, inputString);
    Serial.println("^RWrite OK$");
}

static void httpFlash() {
        WiFiClient client;
        ESPhttpUpdate.rebootOnUpdate(false);
        HTTPUpdateResult ret = ESPhttpUpdate.update(client, inputString);
        switch (ret) {
            case HTTP_UPDATE_FAILED:
                Serial.print("^RFAIL: ");
                Serial.print(ESPhttpUpdate.getLastErrorString());
                Serial.println("$");
                break;
            case HTTP_UPDATE_NO_UPDATES:
                Serial.println("^RNo updated!$");
                break;
            case HTTP_UPDATE_OK:
                Serial.println("^RUpdate OK!$");
                ESP.restart();
                break;
            default:
                Serial.println("^RUpdate unknown error!$");
                break;
        }
}

static void serialProcessCommand() {
    switch (serialCommand) {
        case 'S': // SSID
            processEEPROMCommand(EEPROM_WIFI_SSID);
            break;
        case 'P': // Password
            processEEPROMCommand(EEPROM_WIFI_PASSWORD);
            break;
        case 'N': // NTP
            processEEPROMCommand(EEPROM_NTP_SERVER);
            break;
        case 'T': // TimeZone
            processEEPROMCommand(EEPROM_TIME_ZONE);
            break;
        case 'H': // Hostname
            processEEPROMCommand(EEPROM_OTA_HOSTNAME);
            break;
        case 'O': // OTA password
            processEEPROMCommand(EEPROM_OTA_PASSWORD);
            break;
        case 'C': // Commit
            eepromCommit();
            Serial.println("^RCommit OK$");
            break;
        case 'R': // Reset
            Serial.println("^RReset!$");
            ESP.reset();
            break;
        case 'F': // Flash from HTTP
            httpFlash();
            break;
        default:
            Serial.println("^RUnknown command$");
            break;
    }
}

void serialLoop() {
    while (Serial.available()) {
        char data = Serial.read();

        if (data == '^') {
            serialCommandState = STATE_LOOKING_FOR_COMMAND;
            inputString = "";
            continue;
        }

        switch (serialCommandState) {
            case STATE_LOOKING_FOR_START:
                break;
            case STATE_LOOKING_FOR_COMMAND:
                serialCommand = data;
                serialCommandState = STATE_LOOKING_FOR_END;
                break;
            case STATE_LOOKING_FOR_END:
                if (data == '$') {
                    serialProcessCommand();
                    serialCommandState = STATE_LOOKING_FOR_START;
                    break;
                }
                inputString += data;
                if (inputString.length() > 100) {
                    serialCommandState = STATE_LOOKING_FOR_START;
                    break;
                }
                break;
        }
    }
}
