#include <SPI.h>
#include <Wire.h>
#include <TimeLib.h>
#include <limits.h>

#define RENDER_USE_DELAY // Use delay when rendering instead of just returning for the next cycle

#define MASK_UPPER_DOTS 1
#define MASK_LOWER_DOTS 2

#define DS1307_ADDRESS 0x68
#define ZERO 0x00

#define PIN_LE 10 /* Latch Enabled data accepted while HI level */
#define PIN_HIZ 8 /* Z state in registers outputs (while LOW level) */
#define PIN_DHV 5 /* off/on MAX1771 Driver  Hight Voltage(DHV) 110-220V */
#define PIN_BUZZER 2

#define PIN_LED_RED 9
#define PIN_LED_GREEN 6
#define PIN_LED_BLUE 3

#define PIN_BUTTON_SET A0
#define PIN_BUTTON_UP A2
#define PIN_BUTTON_DOWN A1

#define ANTI_POISON_DELAY 200UL

const uint16_t symbolArray[12] PROGMEM = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 0, 1023}; // Last two chars = none on / all on. They are : and ;
#define getSymbol(idx) (pgm_read_word_near(symbolArray + (idx)))

uint16_t dataToDisplay[6] = {0, 0, 0, 0, 0, 0}; // This will be displayed on tubes
byte dotMask;

bool RTCPresent;
int RTCHours, RTCMinutes, RTCSeconds, RTCDay, RTCMonth, RTCYear, RTCDayOfWeek;
unsigned long RTCLastSyncTime;

String inputString;
unsigned long holdDisplayUntil;
boolean colorSet;
unsigned long holdColorUntil;
byte setR, setG, setB;
unsigned long antiPoisonEnd;

boolean stopwatchEnabled = false;
boolean stopwatchRunning = false;
unsigned long prevMillis;
unsigned long stopwatchTime, countdownTo;

void setup() 
{
  // Pin setup
  pinMode(PIN_DHV, OUTPUT);
  digitalWrite(PIN_DHV, LOW); // Turn off HV ASAP during setup

  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_BLUE, OUTPUT);
  analogWrite(PIN_LED_RED, 0);
  analogWrite(PIN_LED_GREEN, 0);
  analogWrite(PIN_LED_BLUE, 0);

  pinMode(PIN_BUZZER, OUTPUT);

  pinMode(PIN_LE, OUTPUT);
  pinMode(PIN_HIZ, OUTPUT);
  digitalWrite(PIN_HIZ, LOW);

  pinMode(PIN_BUTTON_SET, INPUT_PULLUP);
  pinMode(PIN_BUTTON_UP, INPUT_PULLUP);
  pinMode(PIN_BUTTON_DOWN, INPUT_PULLUP);

  // Lib setup
  Wire.begin();
  Serial.begin(115200);
  SPI.begin();

  // Begin initialization routines
  inputString.reserve(32);

  testRTC();
  RTCLastSyncTime = ULONG_MAX;

  // Turn on HV
  digitalWrite(PIN_DHV, HIGH);
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar != '\n') {
      inputString += inChar;
      if (inputString.length() >= 30) {
        inputString = "";
        Serial.println(F("Line too long. Aborted."));
      }
      continue;
    }
    Serial.print(F("Got "));
    Serial.println(inputString);
    unsigned long addition;

    switch (inputString[0]) {
      // T HH MM SS DD MM YYYY W
      // H = Hours, M = Minutes, S = Seconds, D = Day of month, M = month, Y = year, W = Day of week (ALL Dec)
      // Sets the time on the clock
      // T175630010320180
      case 'T':
        if (inputString.length() < 16) {
          Serial.println(F("T BAD (Invalid length; expected 16)"));
          break;
        }
        RTCHours = int(inputString.substring(1, 3).toInt());
        RTCMinutes = int(inputString.substring(3, 5).toInt());
        RTCSeconds = int(inputString.substring(5, 7).toInt());
        RTCDay = int(inputString.substring(6, 9).toInt());
        RTCMonth = int(inputString.substring(9, 11).toInt());
        RTCYear = int(inputString.substring(11, 15).toInt());
        RTCDayOfWeek = int(inputString.substring(15, 16).toInt());
        setTime(RTCHours, RTCMinutes, RTCSeconds, RTCDay, RTCMonth, RTCYear);
        setRTCDateTime(RTCHours, RTCMinutes, RTCSeconds, RTCDay, RTCMonth, RTCYear, RTCDayOfWeek);
        Serial.println(F("T OK"));
        break;
     // X
     // Performs a display reset of all modes
      case 'X':
        holdDisplayUntil = 0;
        holdColorUntil = 0;
        stopwatchEnabled = false;
        stopwatchRunning = false;
        stopwatchTime = 0;
        countdownTo = 0;
        Serial.println(F("X OK"));
        break;
      // P CC
      // C = Count (Dec)
      // Performs an anti poisoning routine <C> times
      // P01
      case 'P':
        if (inputString.length() < 2) {
          Serial.println(F("P BAD (Invalid length; expected 2)"));
          break;
        }
        displayAntiPoison(inputString.substring(1, 3).toInt());
        Serial.println(F("P OK"));
        break;
      // F MMMMMMMM RR GG BB [D NNNNNN]
      // M = milliseconds (Dec), R = Red (Hex), G = Green (Hex), B = Blue (Hex), D = dots (Bitmask Dec) to show the message, N = Nixie message (Dec)
      // Shows a "flash"/"alert" message on the clock (will show this message instead of the time for <M> milliseconds. Does not use/reset hold when 0). Dots are bit 1 for lower and bit 2 for upper. Turned off when HIGH
      // Will only set color flash if used without D and N parameters
      // If sent without any parameters, resets current flash message and goes back to clock mode
      // F0000100000FF0021337:;
      case 'F':
        if (inputString.length() < 15) {
          if (inputString.length() == 1) {
            holdDisplayUntil = 0;
            holdColorUntil = 0;
            break;
          }
          Serial.println(F("F BAD (Invalid length; expected 22, 15 or 1)"));
          break;
        }

        addition = (unsigned long)inputString.substring(1, 9).toInt();
        if (addition > 0) {
          holdColorUntil = millis() + addition;
        }

        setColor(hexInputToByte(9), hexInputToByte(11), hexInputToByte(13));

        if (inputString.length() >= 22) {
          holdDisplayUntil = holdColorUntil;
          int dots = inputString[15] - '0';
          setDots((dots & 2) == 2, (dots & 1) == 1);
          for (int i = 0; i < 6; i++) {
            dataToDisplay[i] = getSymbol(inputString[i + 16] - '0');
          }
        }

        antiPoisonEnd = 0;

        Serial.println(F("F OK"));
        break;
      // C MMMMMMMM
      // M = Time in ms (Dec)
      // Starts a countdown for <M> ms
      // C00010000
      case 'C':
        addition = (unsigned long)inputString.substring(1, 9).toInt();
        if (addition == 0) {
          countdownTo = 0;
        } else {
          stopwatchEnabled = false;
          stopwatchRunning = false;
          stopwatchTime = 0;
          countdownTo = millis() + addition;
        }
        break;
      // W C
      // C = subcommand
      // Controls the stopwatch. R for reset/disable, P for pause, U for un-pause, S for start/restart
      // WS
      case 'W':
        switch (inputString[1]) {
          case 'R':
            stopwatchEnabled = false;
            stopwatchRunning = false;
            stopwatchTime = 0;
            break;
          case 'P':
            stopwatchRunning = false;
            break;
          case 'U':
            stopwatchRunning = true;
            break;
          case 'S':
            countdownTo = 0;
            stopwatchEnabled = true;
            stopwatchRunning = true;
            stopwatchTime = 0;
        }
        break;
    }

    inputString = "";
  }
}

byte hexInputToByte(int offset) {
  byte msn = (inputString[offset] < '9') ? inputString[offset] - '0' : inputString[offset] - '7';
  ++offset;
  byte lsn = (inputString[offset] < '9') ? inputString[offset] - '0' : inputString[offset] - '7';
  return (msn << 4) + lsn;
}

void setDots(boolean upper, boolean lower) {
  dotMask = (upper ? 0 : MASK_UPPER_DOTS) | (lower ? 0 : MASK_LOWER_DOTS);
}

void setColor(byte r, byte g, byte b) {
  colorSet = true;
  setR = r;
  setG = g;
  setB = b;
  analogWrite(PIN_LED_RED, r);
  analogWrite(PIN_LED_GREEN, g);
  analogWrite(PIN_LED_BLUE, b);
}

void displayAntiPoison(int count) {
  antiPoisonEnd = millis() + (unsigned long)(ANTI_POISON_DELAY * 10UL * count);
}

void loop() {
  unsigned long curMillis = millis();
  unsigned long milliDelta = curMillis - prevMillis;
  if (curMillis < prevMillis) {
    milliDelta = 0;
  }
  prevMillis = curMillis;

  if (RTCPresent && (curMillis - RTCLastSyncTime >= 10000 || curMillis < RTCLastSyncTime)) {
    getRTCTime();
    setTime(RTCHours, RTCMinutes, RTCSeconds, RTCDay, RTCMonth, RTCYear);
    RTCLastSyncTime = curMillis;
  }

  if (colorSet && holdColorUntil <= curMillis) {
    analogWrite(PIN_LED_RED, 0);
    analogWrite(PIN_LED_GREEN, 0);
    analogWrite(PIN_LED_BLUE, 0);
    colorSet = false;
    holdColorUntil = 0;
  }

  if (stopwatchRunning) {
    stopwatchTime += milliDelta;
  }

  if (antiPoisonEnd > curMillis) {
    int idx = (antiPoisonEnd - curMillis) / ANTI_POISON_DELAY;
    while (idx < 0) {
      idx += 10;
    }
    idx %= 10;
    for (int i = 0; i < 6; i++) {
      dataToDisplay[i] = getSymbol(idx);
    }
  } else if (holdDisplayUntil <= curMillis) {
    holdDisplayUntil = curMillis + 10;
    if (countdownTo > 0) {
      if (countdownTo <= curMillis) {
        showShortTime(0);
      } else {
        showShortTime(countdownTo - curMillis);
      }
    } else if (stopwatchEnabled) {
      showShortTime(stopwatchTime);
    } else {
      byte s = second();
      if (s % 2) {
        setDots(true, true);
      } else {
        setDots(false, false);
      }
      byte h = hour();
      insert1(0, h / 10, true);
      insert1(1, h, false);
      insert2(2, minute(), false);
      insert2(4, s, false);
      if (h < 4 && s % 10 == 0) {
        displayAntiPoison(1);
      }
    }
  }
  
  renderNixies();
}

#define ONE_SECOND_IN_MS (1000UL)
#define ONE_MINUTE_IN_MS (ONE_SECOND_IN_MS * 60UL)
#define ONE_HOUR_IN_MS (ONE_MINUTE_IN_MS * 60UL)

void showShortTime(unsigned long timeMs)
{
  bool trimLZ = true;
  if (timeMs >= ONE_HOUR_IN_MS) { // Show H/M/S
    setDots(true, false);
    trimLZ = insert2(0, (timeMs / ONE_HOUR_IN_MS) % 100, trimLZ);
    trimLZ = insert2(2, (timeMs / ONE_MINUTE_IN_MS) % 60, trimLZ);
    insert2(4, (timeMs / ONE_SECOND_IN_MS) % 60, trimLZ);
  } else { // Show M/S/MS
    setDots(false, true);
    trimLZ = insert2(0, (timeMs / ONE_MINUTE_IN_MS) % 60, trimLZ);
    trimLZ = insert2(2, (timeMs / ONE_SECOND_IN_MS) % 60, trimLZ);
    insert2(4, (timeMs / 10UL) % 100, trimLZ);
  }
}

void testRTC() {
  getRTCTime();
  byte prevSeconds = RTCSeconds;
  unsigned long RTCReadingStartTime = millis();
  RTCPresent = true;

  while(prevSeconds == RTCSeconds)
  {
    delay(100);
    getRTCTime();
    if ((millis() - RTCReadingStartTime) > 3000)
    {
      Serial.println(F("Warning! RTC DON'T RESPOND!"));
      RTCPresent = false;
      break;
    }
  }
}

bool insert1(int offset, int data, boolean trimLeadingZero)
{
  data %= 10;
  if (data == 0 && trimLeadingZero) {
    dataToDisplay[offset] = 0;
    return true;
  } else {
    dataToDisplay[offset] = getSymbol(data);
    return false;
  }
}
bool insert2(int offset, int data, boolean trimLeadingZero)
{
  trimLeadingZero = insert1(offset, data / 10, trimLeadingZero);
  return insert1(offset + 1, data, trimLeadingZero);
}

void getRTCTime()
{
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(ZERO);
  Wire.endTransmission();
  
  Wire.requestFrom(DS1307_ADDRESS, 7);
  RTCSeconds = bcdToDec(Wire.read());
  RTCMinutes = bcdToDec(Wire.read());
  RTCHours = bcdToDec(Wire.read() & 0b111111); //24 hour time
  RTCDayOfWeek = bcdToDec(Wire.read()); //0-6 -> sunday - Saturday
  RTCDay = bcdToDec(Wire.read());
  RTCMonth = bcdToDec(Wire.read());
  RTCYear = bcdToDec(Wire.read());
}

void setRTCDateTime(byte h, byte m, byte s, byte d, byte mon, byte y, byte w)
{
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

byte decToBcd(byte val){
  // Convert normal decimal numbers to binary coded decimal
  return ((val / 10) << 4) + (val % 10);
}

byte bcdToDec(byte val)  {
  // Convert binary coded decimal to normal decimal numbers
  return ((val >> 4) * 10) + (val%16);
}

void displaySelfTest()
{
  Serial.println(F("Start LED Test"));

  setDots(true, true);
   
  analogWrite(PIN_LED_RED, 255);
  delay(1000);
  analogWrite(PIN_LED_RED, 0);
  analogWrite(PIN_LED_GREEN, 255);
  delay(1000);
  analogWrite(PIN_LED_GREEN, 0);
  analogWrite(PIN_LED_BLUE, 255);
  delay(1000); 
  analogWrite(PIN_LED_BLUE, 0);
  
  Serial.println(F("Stop LED Test"));

  displayAntiPoison(2);
}

void renderNixies()
{
  static byte anodeGroup = 0;
  static unsigned long lastTimeInterval1Started;

  unsigned long curMicros = micros();
  if (curMicros >= lastTimeInterval1Started) {
    unsigned long timeSinceLastRender = curMicros - lastTimeInterval1Started;
    if (timeSinceLastRender <= 5000) {
#ifdef RENDER_USE_DELAY
      delayMicroseconds(5000 - timeSinceLastRender);
#else // RENDER_USE_DELAY
      return;
#endif // RENDER_USE_DELAY
    }
  }
  lastTimeInterval1Started = curMicros;
  
  byte curTubeL = anodeGroup << 1;
  byte curTubeR = curTubeL + 1;
  
  digitalWrite(PIN_LE, LOW); // allow data input (Transparent mode)

  SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE2));
  SPI.transfer(dotMask);                                                     // [   ][   ][   ][   ][   ][   ][L1 ][L0 ] - L0, L1 - dots
  SPI.transfer(dataToDisplay[curTubeR] >> 6 | 1 << (anodeGroup + 4));        // [   ][A2 ][A1 ][A0 ][RC9][RC8][RC7][RC6] - A0-A2 - anodes
  SPI.transfer(dataToDisplay[curTubeR] << 2 | dataToDisplay[curTubeL] >> 8); // [RC5][RC4][RC3][RC2][RC1][RC0][LC9][LC8] - RC9-RC0 - Right tubes cathodes
  SPI.transfer(dataToDisplay[curTubeL]);                                     // [LC7][LC6][LC5][LC4][LC3][LC2][LC1][LC0] - LC9-LC0 - Left tubes cathodes
  SPI.endTransaction();
  
  digitalWrite(PIN_LE, HIGH); // latching data 
  
  if (++anodeGroup > 2) {
    anodeGroup = 0;
  }
}

