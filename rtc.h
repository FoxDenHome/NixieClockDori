#ifndef _RTC_H_INCLUDED
#define _RTC_H_INCLUDED

void rtcSync();
void rtcSetTime(byte h, byte m, byte s, byte d, byte mon, byte y, byte w);
void rtcTest();

#endif // _RTC_H_INCLUDED

