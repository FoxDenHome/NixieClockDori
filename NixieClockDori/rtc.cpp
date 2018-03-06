#include <DS1307RTC.h>
#include "crcserial.h"
#include "rtc.h"

void rtcSetTime(tmElements_t& tm) {
	setTime(makeTime(tm));
	if (!RTC.chipPresent()) {
		return;
	}
	RTC.write(tm);
}

void rtcInit() {
	const time_t prevT = RTC.get();
	if (!RTC.chipPresent()) {
		serialSendF("< Warning! RTC NOT ON BOARD!");
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
			serialSendF("< Warning! RTC ZERO!");
			return;
		}
	}

	setSyncProvider(RTC.get);
}

