#include <SPI.h>
#include <TimeLib.h>
#include "rtc.h"
#include "config.h"
#include "crcserial.h"

const byte MASK_UPPER_DOTS = 1;
const byte MASK_LOWER_DOTS = 2;
const byte MASK_BOTH_DOTS = MASK_UPPER_DOTS | MASK_LOWER_DOTS;

#ifdef EFFECT_SLOT_MACHINE
#define EFFECT_ENABLED
#endif

/********************/
/* PIN DEFINITIONS  */
/********************/
const byte PIN_DISPLAY_LATCH       = 10; // Passes data from SPI chip to display while HIGH (pulled LOW during SPI write)
const byte PIN_HIZ                 = 8;  // Z state in registers outputs (while LOW level) Always LOW? */
const byte PIN_HIGH_VOLTAGE_ENABLE = 5;  // High Voltage (tube power) on while HIGH
const byte PIN_BUZZER              = 2;  // Piezo buzzer pin

const byte PIN_LED_RED             = 9;  // PWM/analog pin for all red LEDs
const byte PIN_LED_GREEN           = 6;  // PWM/analog pin for all green LEDs
const byte PIN_LED_BLUE            = 3;  // PWM/analog pin for all blue LEDs

const byte PIN_BUTTON_SET          = A0; // "set" button
const byte PIN_BUTTON_UP           = A2; // "up" button
const byte PIN_BUTTON_DOWN         = A1; // "down" button

/*******************/
/* OTHER CONSTANTS */
/*******************/
const unsigned long DISPLAY_DELAY_MICROS = 5000UL;

const unsigned long ONE_SECOND_IN_MS = 1000UL;
const unsigned long ONE_MINUTE_IN_MS = ONE_SECOND_IN_MS * 60UL;
const unsigned long ONE_HOUR_IN_MS = ONE_MINUTE_IN_MS * 60UL;

const uint16_t ALL_TUBES = (1 << 10) - 1; // Bitmask to enable all tubes
const uint16_t NO_TUBES = 0;

/********************/
/* GLOBAL VARIABLES */
/********************/
#ifdef EFFECT_ENABLED
byte dataIsTransitioning[6] = {0, 0, 0, 0, 0, 0};
uint16_t dataToDisplayOld[6] = {10000, 10000, 10000, 10000, 10000, 10000}; // 10000 is a clearly invalid value but fits into a uint16_t
#endif
uint16_t dataToDisplay[6] = {0, 0, 0, 0, 0, 0}; // This will be displayed on tubes
byte dotMask;

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

/****************/
/* PROGRAM CODE */
/****************/
void setup() {
  // Pin setup
  pinMode(PIN_HIGH_VOLTAGE_ENABLE, OUTPUT);
  digitalWrite(PIN_HIGH_VOLTAGE_ENABLE, LOW); // Turn off HV ASAP during setup

  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_BLUE, OUTPUT);
  analogWrite(PIN_LED_RED, 0);
  analogWrite(PIN_LED_GREEN, 0);
  analogWrite(PIN_LED_BLUE, 0);

  pinMode(PIN_BUZZER, OUTPUT);

  pinMode(PIN_DISPLAY_LATCH, OUTPUT);
  pinMode(PIN_HIZ, OUTPUT);
  digitalWrite(PIN_HIZ, LOW);

  pinMode(PIN_BUTTON_SET, INPUT_PULLUP);
  pinMode(PIN_BUTTON_UP, INPUT_PULLUP);
  pinMode(PIN_BUTTON_DOWN, INPUT_PULLUP);

  // Lib setup
  SPI.begin();

  // Begin initialization routines
  serialInit();

  rtcTest();
  rtcSync();

  digitalWrite(PIN_HIGH_VOLTAGE_ENABLE, HIGH);

  prevMillis = millis();
  serialSend(F("< Ready"));
}

void serialEvent() {
  static bool receivedStart = false;

  while (Serial.available()) {
    if (!serialReadNext()) {
      continue;
    }

    byte tmpData;

    switch (inputString[0]) {
      // T HH II SS DD MM YY W
      // H = Hours, I = Minutes, S = Seconds, D = Day of month, M = month, Y = year, W = Day of week (ALL Dec)
      // Sets the time on the clock
      // T1756300103180
      case 'T':
        if (inputString.length() < 14) {
          serialSend(F("T BAD (Invalid length; expected 16)"));
          break;
        }
        tmElements_t tm;
        tm.Hour = inputString.substring(1, 3).toInt();
        tm.Minute = inputString.substring(3, 5).toInt();
        tm.Second = inputString.substring(5, 7).toInt();
        tm.Day = inputString.substring(6, 9).toInt();
        tm.Month = inputString.substring(9, 11).toInt();
        tm.Year = inputString.substring(11, 13).toInt();
        tm.Wday = inputString.substring(13, 14).toInt();
        rtcSetTime(tm);
        serialSend(F("T OK"));
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
        serialSend(F("X OK"));
        break;
      // P CC
      // C = Count (Dec)
      // Performs an anti poisoning routine <C> times
      // P01
      case 'P':
        if (inputString.length() < 2) {
          serialSend(F("P BAD (Invalid length; expected 2)"));
          break;
        }
        displayAntiPoison(inputString.substring(1, 3).toInt());
        serialSend(F("P OK"));
        break;
      // G [MMMMMMMM IIII OOOO RR GG BB]
      // M = milliseconds (Dec), I = Ease-In (Dec), O = Ease-Out (Dec), R = Red (Hex), G = Green (Hex), B = Blue (Hex)
      // Shows a "flash"/"alert" color on the clock
      // If sent without any parameters, resets current flash color
      // G000010000500050000FF00
      case 'G':
        if (inputString.length() < 23) {
          if (inputString.length() < 3) { // Allow for \r\n
            noColor();
            serialSend(F("G OK"));
            break;
          }
          serialSend(F("G BAD (Invalid length; expected 23 or 1)"));
          break;
        }
        holdColorStartTime = millis();
        holdColorEaseInTarget = holdColorStartTime + (unsigned long)inputString.substring(9, 13).toInt();
        holdColorSteadyTarget = holdColorEaseInTarget + (unsigned long)inputString.substring(1, 9).toInt();
        holdColorEaseOutTarget = holdColorSteadyTarget + (unsigned long)inputString.substring(13, 17).toInt();

        setR = hexInputToByte(17);
        setG = hexInputToByte(19);
        setB = hexInputToByte(21);
        colorSet = true;
        serialSend(F("G OK"));
        break;
      // F [MMMMMMMM D NNNNNN]
      // M = milliseconds (Dec), D = dots (Bitmask Dec) to show the message, N = Nixie message (Dec)
      // Shows a "flash"/"alert" message on the clock (will show this message instead of the time for <M> milliseconds. Does not use/reset hold when 0). Dots are bit 1 for lower and bit 2 for upper. Turned off when HIGH
      // If sent without any parameters, resets current flash message and goes back to clock mode
      // F0000100021337NA
      case 'F':
        if (inputString.length() < 16) {
          if (inputString.length() < 3) { // Allow for \r\n
            holdDisplayUntil = 0;
            serialSend(F("F OK"));
            break;
          }
          serialSend(F("F BAD (Invalid length; expected 16 or 1)"));
          break;
        }

        holdDisplayUntil = millis() + (unsigned long)inputString.substring(1, 9).toInt();
        tmpData = inputString[9] - '0';
        setDots((tmpData & 2) == 2, (tmpData & 1) == 1);
        for (byte i = 0; i < 6; i++) {
          tmpData = inputString[i + 10];
          if (tmpData == 'N') {
            dataToDisplay[i] = NO_TUBES;
          } else if (tmpData == 'A') {
            dataToDisplay[i] = ALL_TUBES;
          } else {
            dataToDisplay[i] = getNumber(tmpData - '0');
          }
        }

        antiPoisonEnd = 0;

        serialSend(F("F OK"));
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
        serialSend(F("C OK"));
        break;
      // W C
      // C = subcommand
      // Controls the stopwatch. R for reset/disable, P for pause, U for un-pause, S for start/restart
      // WS
      case 'W':
        if (inputString.length() < 2) {
          serialSend(F("W BAD (Invalid length; expected 2)"));
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
          serialSend(F("W OK"));
        }
        break;
    }

    inputString = "";
  }
}

#define hexCharToNum(c) ((c <= '9') ? c - '0' : c - '7')
byte hexInputToByte(const byte offset) {
  const byte msn = inputString[offset];
  const byte lsn = inputString[offset + 1];
  return (hexCharToNum(msn) << 4) + hexCharToNum(lsn);
}

uint16_t getNumber(const byte idx) {
  return 1 << (idx % 10);
}

//#define setDotsC_false_false() { dotMask = MASK_BOTH_DOTS; }
//#define setDotsC_true_false() { dotMask = MASK_LOWER_DOTS; }
//#define setDotsC_false_true() { dotMask = MASK_UPPER_DOTS; }
//#define setDotsC_true_true() { dotMask = 0; }
//#define setDotsC_conc(upper, lower) setDotsC_##upper##_##lower
//#define setDotsConst(upper, lower) setDotsC_conc(upper, lower)()
#define setDotsConst setDots
void setDots(const bool upper, const bool lower) {
  dotMask = (upper ? 0 : MASK_UPPER_DOTS) | (lower ? 0 : MASK_LOWER_DOTS);
}

void noColor() {
  colorSet = false;
  analogWrite(PIN_LED_RED, 0);
  analogWrite(PIN_LED_GREEN, 0);
  analogWrite(PIN_LED_BLUE, 0);
}

void displayAntiPoison(const unsigned long count) {
  antiPoisonEnd = millis() + (ANTI_POISON_DELAY * 10UL * count);
}

void loop() {
  bool displayDirty = false;
  const unsigned long curMillis = millis();
  const unsigned long milliDelta = (curMillis >= prevMillis) ? (curMillis - prevMillis) : 0;
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
    const uint16_t sym = getNumber((antiPoisonEnd - curMillis) / ANTI_POISON_DELAY);
    for (byte i = 0; i < 6; i++) {
      dataToDisplay[i] = sym;
    }
    displayDirty = false;
  } else if (holdDisplayUntil <= curMillis) {
    holdDisplayUntil = curMillis + 10;
    if (countdownTo > 0) {
      if (countdownTo <= curMillis) {
        displayDirty = showShortTime(0, true);
      } else {
        displayDirty = showShortTime(countdownTo - curMillis, true);
      }
    } else if (stopwatchEnabled) {
      displayDirty = showShortTime(stopwatchTime, true);
    } else {
      const time_t _n = now();
      const byte h = hour(_n);
      const byte s = second(_n);

      if (s % 2) {
        setDotsConst(true, true);
      } else {
        setDotsConst(false, false);
      }

#ifdef CLOCK_TRIM_HOURS
      insert1(0, h / 10, true);
      insert1(1, h, false);
#else
      insert2(0, h, false);
#endif
      insert2(2, minute(_n), false);
      insert2(4, s, false);

      if (h < 4 && s % 10 == 0) {
        displayAntiPoison(1);
      }

      displayDirty = true;
    }
  }

#ifdef EFFECT_ENABLED
  for (byte i = 0; i < 6; i++) {
    if (displayDirty && dataToDisplayOld[i] != dataToDisplay[i]) {
      dataToDisplayOld[i] = dataToDisplay[i];
      dataIsTransitioning[i] = EFFECT_SPEED;
    } else if(dataIsTransitioning[i] > 0) {
      if (dataIsTransitioning[i] > milliDelta) {
        dataIsTransitioning[i] -= milliDelta;
      } else {
        dataIsTransitioning[i] = 0;
      }
    }
  }
#endif

  renderNixies();
}

bool showShortTime(const unsigned long timeMs, bool trimLZ) {
  if (timeMs >= ONE_HOUR_IN_MS) { // Show H/M/S
    setDotsConst(true, false);
    trimLZ = insert2(0, (timeMs / ONE_HOUR_IN_MS) % 100, trimLZ);
    trimLZ = insert2(2, (timeMs / ONE_MINUTE_IN_MS) % 60, trimLZ);
    insert2(4, (timeMs / ONE_SECOND_IN_MS) % 60, trimLZ);
    return true;
  } else { // Show M/S/MS
    setDotsConst(false, true);
    trimLZ = insert2(0, (timeMs / ONE_MINUTE_IN_MS) % 60, trimLZ);
    trimLZ = insert2(2, (timeMs / ONE_SECOND_IN_MS) % 60, trimLZ);
    insert2(4, (timeMs / 10UL) % 100, trimLZ);
    return false; // Don't allow transition effects on rapid timer
  }
}

void insert1(const byte offset, const byte data, const bool trimLeadingZero) {
  if (data == 0 && trimLeadingZero) {
    dataToDisplay[offset] = 0;
  } else {
    dataToDisplay[offset] = getNumber(data);
  }
}

bool insert2(const byte offset, const byte data, const bool trimLeadingZero) {
  insert1(offset, data / 10, trimLeadingZero);
  insert1(offset + 1, data, trimLeadingZero);
  return data == 0;
}

void displaySelfTest() {
  serialSend(F("< Start LED Test"));

  setDotsConst(true, true);

  analogWrite(PIN_LED_RED, 255);
  delay(1000);
  analogWrite(PIN_LED_RED, 0);
  analogWrite(PIN_LED_GREEN, 255);
  delay(1000);
  analogWrite(PIN_LED_GREEN, 0);
  analogWrite(PIN_LED_BLUE, 255);
  delay(1000);
  analogWrite(PIN_LED_BLUE, 0);

  serialSend(F("< Stop LED Test"));

  displayAntiPoison(2);
}

void renderNixies() {
  static byte anodeGroup = 0;
  static unsigned long lastTimeInterval1Started;

  const unsigned long curMicros = micros();
  if (curMicros >= lastTimeInterval1Started) {
    const unsigned long timeSinceLastRender = curMicros - lastTimeInterval1Started;
    if (timeSinceLastRender < DISPLAY_DELAY_MICROS) {
#ifdef RENDER_USE_DELAY
      delayMicroseconds(DISPLAY_DELAY_MICROS - timeSinceLastRender);
#else // RENDER_USE_DELAY
      return;
#endif // RENDER_USE_DELAY
    }
  } else if (curMicros < DISPLAY_DELAY_MICROS) {
#ifdef RENDER_USE_DELAY
    delayMicroseconds(DISPLAY_DELAY_MICROS - curMicros);
#else // RENDER_USE_DELAY
    return;
#endif // RENDER_USE_DELAY
  }
  lastTimeInterval1Started = curMicros;

  const byte curTubeL = anodeGroup << 1;
  const byte curTubeR = curTubeL + 1;

  uint16_t tubeL = dataToDisplay[curTubeL];
  uint16_t tubeR = dataToDisplay[curTubeR];

#ifdef EFFECT_ENABLED
  byte tubeTrans = dataIsTransitioning[curTubeL];
  if (tubeTrans > 0) {
#ifdef EFFECT_SLOT_MACHINE
    tubeL = getNumber(tubeTrans / (EFFECT_SPEED / 10));
#endif
  }

  tubeTrans = dataIsTransitioning[curTubeR];
  if (tubeTrans > 0) {
#ifdef EFFECT_SLOT_MACHINE
    tubeR = getNumber(tubeTrans / (EFFECT_SPEED / 10));
#endif
  }
#endif

  digitalWrite(PIN_DISPLAY_LATCH, LOW);
  SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE2));
  SPI.transfer(dotMask);                            // [   ][   ][   ][   ][   ][   ][L1 ][L0 ] - L0     L1 - dots
  SPI.transfer(tubeR >> 6 | 1 << (anodeGroup + 4)); // [   ][A2 ][A1 ][A0 ][RC9][RC8][RC7][RC6] - A0  -  A2 - anodes
  SPI.transfer(tubeR << 2 | tubeL >> 8);            // [RC5][RC4][RC3][RC2][RC1][RC0][LC9][LC8] - RC9 - RC0 - Right tubes cathodes
  SPI.transfer(tubeL);                              // [LC7][LC6][LC5][LC4][LC3][LC2][LC1][LC0] - LC9 - LC0 - Left tubes cathodes
  SPI.endTransaction();
  digitalWrite(PIN_DISPLAY_LATCH, HIGH);

  if (++anodeGroup > 2) {
    anodeGroup = 0;
  }
}

