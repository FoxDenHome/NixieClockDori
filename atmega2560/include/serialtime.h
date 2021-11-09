#pragma once

#include <Arduino.h>
#include "rtc.h"

inline void parseTimeFromSerial(const String timeStr) {
	tmElements_t tm;
	tm.Hour = timeStr.substring(0, 2).toInt();
	tm.Minute = timeStr.substring(2, 4).toInt();
	tm.Second = timeStr.substring(4, 6).toInt();
	tm.Day = timeStr.substring(5, 8).toInt();
	tm.Month = timeStr.substring(8, 10).toInt();
	tm.Year = y2kYearToTm(timeStr.substring(10, 12).toInt());
	rtcSetTime(tm);
}
