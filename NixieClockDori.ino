#include <SPI.h>
#include <Wire.h>
#include <TimeLib.h>
#include "rtc.h"
#include "config.h"

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

#define ONE_SECOND_IN_MS (1000UL)
#define ONE_MINUTE_IN_MS (ONE_SECOND_IN_MS * 60UL)
#define ONE_HOUR_IN_MS (ONE_MINUTE_IN_MS * 60UL)

const uint16_t symbolArray[12] PROGMEM = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 0, 1023}; // Last two chars = none on / all on. They are : and ;
#define getSymbol(idx) (pgm_read_word_near(symbolArray + (idx)))

uint16_t dataToDisplay[6] = {0, 0, 0, 0, 0, 0}; // This will be displayed on tubes
byte dotMask;

String inputString;
unsigned long holdDisplayUntil;
bool colorSet;

unsigned long holdColorStartTime, holdColorEaseInTarget, holdColorSteadyTarget, holdColorEaseOutTarget;
byte setR, setG, setB;

unsigned long antiPoisonEnd;

bool stopwatchEnabled = false;
bool stopwatchRunning = false;
unsigned long prevMillis;
unsigned long stopwatchTime, countdownTo;

unsigned long RTCLastSyncTime;

void setup() {
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

  rtcTest();
  rtcSync();

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

    byte tmpData;

    switch (inputString[0]) {
      // T HH MM SS DD MM YY W
      // H = Hours, M = Minutes, S = Seconds, D = Day of month, M = month, Y = year, W = Day of week (ALL Dec)
      // Sets the time on the clock
      // T1756300103180
      case 'T':
        if (inputString.length() < 16) {
          Serial.println(F("T BAD (Invalid length; expected 16)"));
          break;
        }
        rtcSetTime(
          inputString.substring(1, 3).toInt(),
          inputString.substring(3, 5).toInt(),
          inputString.substring(5, 7).toInt(),
          inputString.substring(6, 9).toInt(),
          inputString.substring(9, 11).toInt(),
          inputString.substring(11, 35).toInt(),
          inputString.substring(13, 14).toInt()
        );
        Serial.println(F("T OK"));
        break;
      // X
      // Performs a display reset of all modes
      case 'X':
        holdDisplayUntil = 0;
        noColor();
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
      // G MMMMMMMM IIII OOOO [RR GG BB]
      // M = milliseconds (Dec), I = Ease-In (Dec), O = Ease-Out (Dec), R = Red (Hex), G = Green (Hex), B = Blue (Hex)
      // Shows a "flash"/"alert" message on the clock (will show this message instead of the time for <M> milliseconds. Does not use/reset hold when 0). Dots are bit 1 for lower and bit 2 for upper. Turned off when HIGH
      // If sent without any parameters, resets current flash message and goes back to clock mode
      // G000010000500050000FF00
      case 'G':
        if (inputString.length() < 15) {
          if (inputString.length() == 1) {
            noColor();
            break;
          }
          Serial.println(F("G BAD (Invalid length; expected 15 or 1)"));
          break;
        }
        holdColorStartTime = millis();
        holdColorEaseInTarget = holdColorStartTime + (unsigned long)inputString.substring(9, 13).toInt();
        holdColorSteadyTarget = holdColorEaseInTarget + (unsigned long)inputString.substring(1, 9).toInt();
        holdColorEaseOutTarget = holdColorSteadyTarget + (unsigned long)inputString.substring(13, 17).toInt();

        tmpData = inputString[17] - '0';
        setR = hexInputToByte(18);
        setG = hexInputToByte(20);
        setB = hexInputToByte(22);
        colorSet = true;
        Serial.println(F("G OK"));
        break;
      // F MMMMMMMM [D NNNNNN]
      // M = milliseconds (Dec), D = dots (Bitmask Dec) to show the message, N = Nixie message (Dec)
      // Shows a "flash"/"alert" color on the clock
      // If sent without any parameters, resets current flash message and goes back to clock mode
      // F0000100021337:;
      case 'F':
        if (inputString.length() < 16) {
          if (inputString.length() == 1) {
            holdDisplayUntil = 0;
            break;
          }
          Serial.println(F("F BAD (Invalid length; expected 16 or 1)"));
          break;
        }

        holdDisplayUntil = millis() + (unsigned long)inputString.substring(1, 9).toInt();
        tmpData = inputString[9] - '0';
        setDots((tmpData & 2) == 2, (tmpData & 1) == 1);
        for (int i = 0; i < 6; i++) {
          dataToDisplay[i] = getSymbol(inputString[i + 10] - '0');
        }

        antiPoisonEnd = 0;

        Serial.println(F("F OK"));
        break;
      // C MMMMMMMM
      // M = Time in ms (Dec)
      // Starts a countdown for <M> ms. Stops countdown if <M> = 0
      // C00010000
      case 'C':
        if (inputString.length() < 9) {
          countdownTo = 0;
        } else {
          stopwatchEnabled = false;
          stopwatchRunning = false;
          stopwatchTime = 0;
          countdownTo = millis() + inputString.substring(1, 9).toInt();
        }
        Serial.println(F("C OK"));
        break;
      // W C
      // C = subcommand
      // Controls the stopwatch. R for reset/disable, P for pause, U for un-pause, S for start/restart
      // WS
      case 'W':
        if (inputString.length() < 2) {
          Serial.println(F("W BAD (Invalid length; expected 2)"));
          break;
        }
        tmpData = true;
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
            break;
          default:
            tmpData = false;
            Serial.print(F("W BAD (Invalid C)"));
            break;
        }
        if (tmpData) {
          Serial.println(F("W OK"));
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

void setDots(bool upper, bool lower) {
  dotMask = (upper ? 0 : MASK_UPPER_DOTS) | (lower ? 0 : MASK_LOWER_DOTS);
}

void noColor() {
  colorSet = false;
  analogWrite(PIN_LED_RED, 0);
  analogWrite(PIN_LED_GREEN, 0);
  analogWrite(PIN_LED_BLUE, 0);
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

  if (curMillis - RTCLastSyncTime >= 10000 || curMillis < RTCLastSyncTime) {
    rtcSync();
    RTCLastSyncTime = curMillis;
  }

  // Handle color logic
  if (colorSet) {
    float factor = 1.0;
    if (curMillis < holdColorEaseInTarget) {
      factor = 1.0 - ((float)(holdColorEaseInTarget - curMillis) / (float)(holdColorEaseInTarget - holdColorStartTime));
    } else if (curMillis > holdColorEaseOutTarget) {
      colorSet = false;
      factor = 0.0;
    } else if (curMillis > holdColorSteadyTarget) {
      factor = (float)(holdColorEaseOutTarget - curMillis) / (float)(holdColorEaseOutTarget - holdColorSteadyTarget);
    }
    analogWrite(PIN_LED_RED, setR * factor);
    analogWrite(PIN_LED_GREEN, setG * factor);
    analogWrite(PIN_LED_BLUE, setB * factor);
  }

  // Handle other stuff
  if (stopwatchRunning) {
    stopwatchTime += milliDelta;
  }

  // Handle "what to display" logic
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
#ifdef CLOCK_TRIM_HOURS
      insert1(0, h / 10, true);
      insert1(1, h, false);
#else
      insert2(0, h, false);
#endif
      insert2(2, minute(), false);
      insert2(4, s, false);
      if (h < 4 && s % 10 == 0) {
        displayAntiPoison(1);
      }
    }
  }

  renderNixies();
}

void showShortTime(unsigned long timeMs) {
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

bool insert1(int offset, int data, bool trimLeadingZero) {
  data %= 10;
  if (data == 0 && trimLeadingZero) {
    dataToDisplay[offset] = 0;
    return true;
  } else {
    dataToDisplay[offset] = getSymbol(data);
    return false;
  }
}

bool insert2(int offset, int data, bool trimLeadingZero) {
  trimLeadingZero = insert1(offset, data / 10, trimLeadingZero);
  return insert1(offset + 1, data, trimLeadingZero);
}

void displaySelfTest() {
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

void renderNixies() {
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
  SPI.transfer(dotMask);                                                     // [   ][   ][   ][   ][   ][   ][L1 ][L0 ] - L0     L1 - dots
  SPI.transfer(dataToDisplay[curTubeR] >> 6 | 1 << (anodeGroup + 4));        // [   ][A2 ][A1 ][A0 ][RC9][RC8][RC7][RC6] - A0  -  A2 - anodes
  SPI.transfer(dataToDisplay[curTubeR] << 2 | dataToDisplay[curTubeL] >> 8); // [RC5][RC4][RC3][RC2][RC1][RC0][LC9][LC8] - RC9 - RC0 - Right tubes cathodes
  SPI.transfer(dataToDisplay[curTubeL]);                                     // [LC7][LC6][LC5][LC4][LC3][LC2][LC1][LC0] - LC9 - LC0 - Left tubes cathodes
  SPI.endTransaction();

  digitalWrite(PIN_LE, HIGH); // latching data

  if (++anodeGroup > 2) {
    anodeGroup = 0;
  }
}

