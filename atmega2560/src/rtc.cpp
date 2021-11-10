#include <Arduino.h>
#include <TimeLib.h>
#include <DS3232RTC.h>

#include "rtc.h"
#include "variables.h"

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
		hostSerial.send("< Warning! Unable to sync with RTC! Try setting the time?");
	}
}

