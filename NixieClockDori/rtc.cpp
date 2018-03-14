#include <DS3232RTC.h>
#include "crcserial.h"
#include "rtc.h"

void rtcSetTime(tmElements_t& tm) {
	setTime(makeTime(tm));
	RTC.write(tm);
}

void rtcInit() {
	RTC.begin();

	const time_t prevT = RTC.get();

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
	if (timeStatus() != timeSet) {
		serialSendF("< Warning! Unable to sync with RTC!");
	}
}

