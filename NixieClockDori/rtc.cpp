#include <DS3232RTC.h>
#include "crcserial.h"
#include "rtc.h"

void rtcSetTime(tmElements_t& tm) {
	const time_t t = makeTime(tm);
	setTime(t);
	RTC.set(t);
}

void rtcInit() {
	setSyncProvider(RTC.get);
	if (timeStatus() != timeSet) {
		serialSendF("< Warning! Unable to sync with RTC! Try setting the time?");
	}
}

