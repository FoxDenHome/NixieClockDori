#include <DS3232RTC.h>
#include "crcserial.h"
#include "rtc.h"

void rtcSetTimeRaw(const time_t t) {
	setTime(t);
	RTC.set(t);
}

void rtcSetTime(tmElements_t& tm) {
	rtcSetTimeRaw(makeTime(tm));
}

void rtcInit() {
	setSyncProvider(RTC.get);
	if (timeStatus() != timeSet) {
		serialSendF("< Warning! Unable to sync with RTC! Try setting the time?");
	}
}

