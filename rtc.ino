#include <TimeLib.h>

#define DS1307_ADDRESS 0x68
#define ZERO 0

bool rtcPresent;

void rtcSync() {
  if (!rtcPresent) {
    return;
  }

  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(ZERO);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 7);
  const byte s = bcdToDec(Wire.read());
  const byte m = bcdToDec(Wire.read());
  const byte h = bcdToDec(Wire.read() & 0b111111); //24 hour time
  const byte w = bcdToDec(Wire.read()); //0-6 -> sunday - Saturday
  const byte d = bcdToDec(Wire.read());
  const byte mon = bcdToDec(Wire.read());
  const byte y = bcdToDec(Wire.read());

  setTime(h, m, s, d, mon, y);
}

void rtcSetTime(const byte h, const byte m, const byte s, const byte d, const byte mon, const byte y, const byte w) {
  setTime(h, m, s, d, mon, y);
  if (!rtcPresent) {
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
  const byte s = bcdToDec(Wire.read());
  Wire.read(); Wire.read(); Wire.read(); Wire.read(); Wire.read(); Wire.read();
  return s;
}

void rtcTest() {
  const byte prevSeconds = rtcReadSeconds();
  const unsigned long rtcReadingStartTime = millis();
  rtcPresent = true;

  do {
    delay(100);
    if ((millis() - rtcReadingStartTime) > 3000) {
      Serial.println(F("^< Warning! RTC DIDN'T RESPOND!"));
      rtcPresent = false;
      break;
    }
  } while (prevSeconds == rtcReadSeconds());
}

byte decToBcd(const byte val) {
  // Convert normal decimal numbers to binary coded decimal
  return ((val / 10) << 4) + (val % 10);
}

byte bcdToDec(const byte val)  {
  // Convert binary coded decimal to normal decimal numbers
  return ((val >> 4) * 10) + (val % 16);
}

