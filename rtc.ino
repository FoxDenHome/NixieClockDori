#include <TimeLib.h>

bool RTCPresent;

void rtcSync() {
  if (!RTCPresent) {
    return;
  }

  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(ZERO);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 7);
  byte RTCSeconds = bcdToDec(Wire.read());
  byte RTCMinutes = bcdToDec(Wire.read());
  byte RTCHours = bcdToDec(Wire.read() & 0b111111); //24 hour time
  byte RTCDayOfWeek = bcdToDec(Wire.read()); //0-6 -> sunday - Saturday
  byte RTCDay = bcdToDec(Wire.read());
  byte RTCMonth = bcdToDec(Wire.read());
  byte RTCYear = bcdToDec(Wire.read());

  setTime(RTCHours, RTCMinutes, RTCSeconds, RTCDay, RTCMonth, RTCYear);
}

void rtcSetTime(byte h, byte m, byte s, byte d, byte mon, byte y, byte w) {
  setTime(h, m, s, d, mon, y);
  if (!RTCPresent) {
    return;
  }

  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(ZERO); //stop Oscillator

  Wire.write(decToBcd(s));
  Wire.write(decToBcd(m));
  Wire.write(decToBcd(h));
  Wire.write(decToBcd(w));
  Wire.write(decToBcd(d));
  Wire.write(decToBcd(mon));
  Wire.write(decToBcd(y));

  Wire.write(ZERO); //start
  Wire.endTransmission();

}

byte rtcReadSeconds() {
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(ZERO);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 7);
  byte RTCSeconds = bcdToDec(Wire.read());
  Wire.read(); Wire.read(); Wire.read(); Wire.read(); Wire.read(); Wire.read();
  return RTCSeconds;
}

void rtcTest() {
  byte prevSeconds = rtcReadSeconds();
  unsigned long RTCReadingStartTime = millis();
  RTCPresent = true;

  do {
    delay(100);
    if ((millis() - RTCReadingStartTime) > 3000) {
      Serial.println(F("Warning! RTC DON'T RESPOND!"));
      RTCPresent = false;
      break;
    }
  } while (prevSeconds == rtcReadSeconds());
}

byte decToBcd(byte val) {
  // Convert normal decimal numbers to binary coded decimal
  return ((val / 10) << 4) + (val % 10);
}

byte bcdToDec(byte val)  {
  // Convert binary coded decimal to normal decimal numbers
  return ((val >> 4) * 10) + (val % 16);
}

