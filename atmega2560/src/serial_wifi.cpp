#include <Arduino.h>

#include "config.h"
#include "serial_wifi.h"
#include "rtc.h"
#include "serialtime.h"

#include "variables.h"

WifiSerial::WifiSerial(HardwareSerial& serial) : CommandSerial(serial) {

}

void WifiSerial::handle() {
    switch (this->command) {
        case '<': // Echo
            hostSerial.echoFirst(F("WiFi: "));
            break;
        case 'T': // Time
            if (this->buffer.length() < 12) {
                hostSerial.echoFirst(F("NTP BAD: "));
                break;
            }
            parseTimeFromSerial(this->buffer);
            hostSerial.echoFirst(F("NTP: "));
            break;
        case '$': // Respond
            hostSerial.sendFirst(F("$N"));
            break;
    }
    hostSerial.sendEnd(this->buffer);
}
