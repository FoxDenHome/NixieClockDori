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
            hostSerial.sendEnd(this->buffer);
            break;
        case '$': // Respond
            hostSerial.sendFirst(F("$N"));
            hostSerial.sendEnd(this->buffer);
            break;
        default:
            HostSerial::handle();
            break;
    }
}

int WifiSerial::type() {
    return 2;
}
