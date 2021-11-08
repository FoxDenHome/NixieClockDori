#pragma once

#include <TimeLib.h>

void rtcSetTimeRaw(const time_t t);
void rtcSetTime(tmElements_t& tm);
void rtcInit();

