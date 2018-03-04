#include <TimeLib.h>
#include <DS1307RTC.h>
#include "crcserial.h"
#include "rtc.h"

void setTimeTM(const tmElements_t& tm) {
	setTime(tm.Hour, tm.Minute, tm.Second, tm.Day, tm.Month, tm.Year);
}

void rtcSetTime(tmElements_t& tm) {
	setTimeTM(tm);
	if (!RTC.chipPresent()) {
		return;
	}
	RTC.write(tm);
}

void rtcInit() {
	const time_t prevT = RTC.get();
	if (!RTC.chipPresent()) {
		serialSendSimple(F("< Warning! RTC NOT ON BOARD!"));
		return;
	}

	if (prevT == 0) {
		RTC.set(5); // Set dummy time
		delay(1000);
		for (byte b = 0; b < 10 && RTC.get() <= 5; b++) {
			delay(100);
		}
		delay(100);
		if (RTC.get() <= 5) {
			serialSendSimple(F("< Warning! RTC ZERO!"));
			return;
		}
	}

	setSyncProvider(RTC.get);
}

