#include <TimeLib.h>
#include <DS1307RTC.h>
#include "crcserial.h"
#include "rtc.h"

void setTimeTM(const tmElements_t& tm) {
  setTime(tm.Hour, tm.Minute, tm.Second, tm.Day, tm.Month, tm.Year);
}

void rtcSetTime(tmElements_t& tm) {
  setTimeTM(tm);
  if (!RTC.chipPresent()) {
    return;
  }
  RTC.write(tm);
}

void rtcInit() {
  time_t prevT = RTC.get();
  if (!RTC.chipPresent()) {
    serialSend(F("< Warning! RTC NOT ON BOARD!"));
    return;
  }

  if (prevT == 0) {
    RTC.set(5); // Set dummy time
    prevT = RTC.get();
  }

  const unsigned long rtcReadingStartTime = millis();

  do {
    delay(100);
    if ((millis() - rtcReadingStartTime) > 3000) {
      serialSend(F("< Warning! RTC DIDN'T RESPOND!"));
      return;
    }
  } while (prevT == RTC.get());

  setSyncProvider(RTC.get);
}

