#include <Arduino.h>
#include <TimeLib.h>
#include <DS3232RTC.h>

#include "rtc.h"
#include "variables.h"

DS3232RTC cRTC;

void rtcSetTimeRaw(const time_t t) {
	setTime(t);
	cRTC.set(t);
}

void rtcSetTime(tmElements_t& tm) {
	rtcSetTimeRaw(makeTime(tm));
}

void rtcInit() {
	cRTC.begin();
	setSyncProvider(cRTC.get);
	if (timeStatus() != timeSet) {
		hostSerial.echo(F("Warning! Unable to sync with RTC! Try setting the time?"));
	}
}

