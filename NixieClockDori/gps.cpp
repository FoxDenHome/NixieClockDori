#include <Adafruit_GPS.h>

#include "gps.h"
#include "crcserial.h"

#ifdef __AVR_ATmega1280__
#define GPS_SERIAL_PORT Serial1
#endif
#ifdef __AVR_ATmega2560__
#define GPS_SERIAL_PORT Serial1
#endif
#ifdef GPS_SERIAL_PORT
#define GPS_ENABLE
#endif

#ifdef GPS_ENABLE
Adafruit_GPS GPS(&GPS_SERIAL_PORT);
#endif

bool gpsToSerial = false;

void gpsInit() {
#ifdef GPS_ENABLE
	GPS.begin(9600);
#endif
}

void gpsLoop() {
#ifdef GPS_ENABLE
	GPS.read();
	if (GPS.newNMEAreceived()) {
		char *nmea = GPS.lastNMEA();
		if (!GPS.parse(nmea)) {
			return;
		}
		if (gpsToSerial) {
			serialSend(nmea + 1);
			if (GPS.year > 0) {
				serialSend(String(GPS.year));
			}
		}
	}
#endif
}

void gpsSendDebug() {
#ifdef GPS_ENABLE
	serialSendN("G ", String(GPS.hour), " ", String(GPS.minute), " ", String(GPS.seconds), " ", String(GPS.year), " ", String(GPS.month), " ", String(GPS.day));
#endif
}
