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
            hostSerial.sendFirst(F("<WiFi: "));
            break;
        case 'T': // Time
            if (this->buffer.length() < 12) {
                hostSerial.sendFirst(F("<NTP BAD: "));
                break;
            }
            parseTimeFromSerial(this->buffer);
            hostSerial.sendFirst(F("<NTP: "));
            break;
        case 'R': // Respond
            hostSerial.sendFirst(F("RN"));
            break;
    }
    hostSerial.sendEnd(this->buffer);
}
