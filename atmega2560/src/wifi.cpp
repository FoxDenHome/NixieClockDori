#include <Arduino.h>

#include "config.h"
#include "wifi.h"
#include "crcserial.h"
#include "rtc.h"
#include "serialtime.h"

#define STATE_LOOKING_FOR_START 0
#define STATE_LOOKING_FOR_COMMAND 1
#define STATE_LOOKING_FOR_END 2

byte wifiCommandState;
char wifiCommand;
String wifiBuffer;

void wifiInit() {
    WIFI_SERIAL.begin(115200);
    wifiBuffer.reserve(128);
    wifiBuffer = "";
    wifiCommandState = STATE_LOOKING_FOR_START;
}

static void wifiProcessCommand() {
    switch (wifiCommand) {
        case 'E':
            serialSendFirst(F("< WiFi: "));
            serialSendNext(wifiBuffer);
            serialSendEnd();
            break;
        case 'T':
            if (wifiBuffer.length() < 12) {
                serialSendFirst(F("< NTP BAD: "));
                serialSendNext(wifiBuffer);
                serialSendEnd();
                break;
            }
            parseTimeFromSerial(wifiBuffer);
            serialSendFirst(F("< NTP: "));
            serialSendNext(wifiBuffer);
            serialSendEnd();
            break;
    }
}

void wifiLoop() {
    while (WIFI_SERIAL.available()) {
        char data = WIFI_SERIAL.read();

        if (data == '^') {
            wifiCommandState = STATE_LOOKING_FOR_COMMAND;
            wifiBuffer = "";
            continue;
        }

        switch (wifiCommandState) {
            case STATE_LOOKING_FOR_START:
                break;
            case STATE_LOOKING_FOR_COMMAND:
                wifiCommand = data;
                wifiCommandState = STATE_LOOKING_FOR_END;
                break;
            case STATE_LOOKING_FOR_END:
                if (data == '$') {
                    wifiProcessCommand();
                    wifiCommandState = STATE_LOOKING_FOR_START;
                    break;
                }
                wifiBuffer += data;
                if (wifiBuffer.length() > 100) {
                    wifiCommandState = STATE_LOOKING_FOR_START;
                    break;
                }
                break;
        }
    }
}
