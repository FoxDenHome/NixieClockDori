#include <DS3232RTC.h>
#include "crcserial.h"
#include "rtc.h"

void rtcSetTime(tmElements_t& tm) {
	setTime(makeTime(tm));
	RTC.write(tm);
}

void rtcInit() {
	setSyncProvider(RTC.get);
	if (timeStatus() != timeSet) {
		serialSendF("< Warning! Unable to sync with RTC! Try setting the time?");
	}
}

