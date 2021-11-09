#include <Arduino.h>

#include "config.h"

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

static void serialOK() {
    Serial.println("^ROK$");
}

void serialSend(const String& str) {
    Serial.print('^');
    Serial.print(inputString);
    Serial.println('$');
}

static void serialProcessCommand() {
    switch (serialCommand) {
        case 'S': // SSID
            break;
        case 'P': // Password
            break;
        case 'H': // Hostname
            break;
        case 'O': // OTA password
            break;
        case 'N': // NTP
            break;
        case 'T': // TimeZone
            break;
        default:
            Serial.println("^RFAIL$");
            return;
    }
    serialOK();
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
