#include <Arduino.h>

#include "config.h"
#include "serial_wifi.h"
#include "rtc.h"

#include "variables.h"

WifiSerial::WifiSerial(HardwareSerial& serial) : HostSerial(serial) {

}

void WifiSerial::handle() {
    switch (this->command) {
        case '<': // Echo
            hostSerial.echoFirst(F("WiFi: "));
            break;
        case '$': // Respond
            hostSerial.sendFirst(F("$N"));
            break;
        default:
            HostSerial::handle();
            return;
    }
    hostSerial.sendEnd(this->buffer);
}
