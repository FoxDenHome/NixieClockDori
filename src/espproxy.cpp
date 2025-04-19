#include "config.h"

#ifdef ENABLE_ESPPROXY

#include "espproxy.h"
#include "serial.h"
#include "variables.h"
#include "DisplayDriver.h"

bool ESPPRoxyMode = false;

void initESPProxy(CommandSerial *serial, uint8_t init) {
    ESPPRoxyMode = true;

    if (serial->type() == hostSerial.type()) {
        WIFI_SERIAL.write(init);
    } else if (serial->type() == wifiSerial.type()) {
        HOST_SERIAL.write(init);
    }

	displayDriverBlank();
}

void loopESPProxy() {
    while (HOST_SERIAL.available()) {
        WIFI_SERIAL.write(HOST_SERIAL.read());
    }

    while (WIFI_SERIAL.available()) {
        HOST_SERIAL.write(WIFI_SERIAL.read());
    }
}

#endif
