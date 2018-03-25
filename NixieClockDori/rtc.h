#pragma once

#include <TimeLib.h>

void rtcSetTimeRaw(time_t t);
void rtcSetTime(tmElements_t& tm);
void rtcInit();

