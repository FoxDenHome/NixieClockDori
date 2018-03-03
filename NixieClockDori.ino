#include <SPI.h>
#include <TimeLib.h>

#include <SoftTimer.h>
#undef PREVENT_LOOP_ITERATION

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

unsigned long holdColorStartTime, holdColorEaseInTarget, holdColorSteadyTarget, holdColorEaseOutTarget;
byte setR, setG, setB;

unsigned long antiPoisonEnd;

bool stopwatchRunning = false;
unsigned long stopwatchTime, countdownTo, holdDisplayUntil;

/****************/
/* PROGRAM CODE */
/****************/

/********************/
/* FUNCTION ALIASES */
/********************/

void updateDisplayCountdown(Task *me);
void updateDisplayStopwatch(Task *me);
void updateDisplayAntiPoison(Task *me);
void updateDisplayClock(Task *me);
void updateColors(Task *me);
void renderNixies(Task *me);
void changeUpdater(Task *me);
void serialReader(Task *me);

Task T_updateDisplayCountdown(5, updateDisplayCountdown);
Task T_updateDisplayStopwatch(5, updateDisplayStopwatch);
Task T_updateDisplayAntiPoison(10, updateDisplayAntiPoison);
Task T_updateDisplayClock(10, updateDisplayClock);
Task T_updateColors(16, updateColors);
Task T_renderNixies(5, renderNixies);
Task T_changeUpdater(5000, changeUpdater);
Task T_serialReader(0, serialReader);

#ifdef EFFECT_ENABLED
void displayEffectsUpdate(Task *me);
Task T_displayEffectsUpdate(5, displayEffectsUpdate);
#endif

//#define setDotsC_false_false() { dotMask = MASK_BOTH_DOTS; }
//#define setDotsC_true_false() { dotMask = MASK_LOWER_DOTS; }
//#define setDotsC_false_true() { dotMask = MASK_UPPER_DOTS; }
//#define setDotsC_true_true() { dotMask = 0; }
//#define setDotsC_conc(upper, lower) setDotsC_##upper##_##lower
//#define setDotsConst(upper, lower) setDotsC_conc(upper, lower)()
#define setDotsConst setDots

/**************************/
/* ARDUINO EVENT HANDLERS */
/**************************/
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
  rtcInit();

  digitalWrite(PIN_HIGH_VOLTAGE_ENABLE, HIGH);

  SoftTimer.add(&T_renderNixies);
  SoftTimer.add(&T_serialReader);
  SoftTimer.add(&T_changeUpdater);
  changeUpdater(NULL);
  
  serialSend(F("< Ready"));
}

void serialReader(Task *me) {
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
        noColor();
        stopwatchRunning = false;
        stopwatchTime = 0;
        countdownTo = 0;
        changeUpdater(NULL);
        serialSend(F("X OK"));
        break;
      // P CC
      // C = Count (Dec)
      // Performs an anti poisoning routine <C> times
      // ^P01|-20043
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
      // ^G000010000500050000FF00|-18506
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
        SoftTimer.add(&T_updateColors);
        serialSend(F("G OK"));
        break;
      // F [MMMMMMMM D NNNNNN]
      // M = milliseconds (Dec), D = dots (Bitmask Dec) to show the message, N = Nixie message (Dec)
      // Shows a "flash"/"alert" message on the clock (will show this message instead of the time for <M> milliseconds. Does not use/reset hold when 0). Dots are bit 1 for lower and bit 2 for upper. Turned off when HIGH
      // If sent without any parameters, resets current flash message and goes back to clock mode
      // ^F0000100021337NA|-15360
      case 'F':
        if (inputString.length() < 16) {
          if (inputString.length() < 3) { // Allow for \r\n
            changeUpdater(NULL);
            serialSend(F("F OK"));
            break;
          }
          serialSend(F("F BAD (Invalid length; expected 16 or 1)"));
          break;
        }

        holdDisplayUntil = millis() + (unsigned long)inputString.substring(1, 9).toInt(); // TODO
        
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

        changeUpdater(NULL);
        serialSend(F("F OK"));
        break;
      // C MMMMMMMM
      // M = Time in ms (Dec)
      // Starts a countdown for <M> ms. Stops countdown if <M> = 0
      // ^C00010000|9735
      // ^C|-26281
      case 'C':
        if (inputString.length() < 9) {
          countdownTo = 0;
        } else {
          countdownTo = millis() + inputString.substring(1, 9).toInt();
        }
        changeUpdater(NULL);
        serialSend(F("C OK"));
        break;
      // W C
      // C = subcommand
      // Controls the stopwatch. R for reset/disable, P for pause, U for un-pause, S for start/restart
      // ^WS|-8015
      // ^WR|-3952
      case 'W':
        if (inputString.length() < 2) {
          serialSend(F("W BAD (Invalid length; expected 2)"));
          break;
        }
        tmpData = true;
        switch (inputString[1]) {
          case 'R':
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
            stopwatchRunning = true;
            stopwatchTime = 1;
            break;
          default:
            tmpData = false;
            Serial.print(F("W BAD (Invalid C)"));
            break;
        }
        if (tmpData) {
          changeUpdater(NULL);
          serialSend(F("W OK"));
        }
        break;
    }
  }
}

/********************/
/* DISPLAY UPDATERS */
/********************/
byte nextTaskHiPri = 0;
Task* getNextDisplayTaskHiPri(byte initialTask) {
  do {
    switch (nextTaskHiPri++) {
      case 0:
        if (stopwatchTime <= 0) {
          break;
        }
        return &T_updateDisplayStopwatch;
      case 1:
        if (countdownTo <= 0) {
          break;
        }
        return &T_updateDisplayCountdown;
      default:
        nextTaskHiPri = 0;
        break;
    }
  } while(nextTaskHiPri != initialTask);
  return NULL;
}

byte nextTaskLoPri = 0;
Task* getNextDisplayTaskLoPri(byte initialTask) {
  return &T_updateDisplayClock; // Currently, only clock present as LowPri
}

void changeUpdater(Task *me) {
  static Task *lastTask;
  Task *newTask;
  if (holdDisplayUntil > millis() || antiPoisonEnd > millis()) {
    if (lastTask) {
      SoftTimer.remove(lastTask);
      lastTask = NULL;
    }
    return;
  }

  newTask = getNextDisplayTaskHiPri(nextTaskHiPri);
  if (!newTask) {
    newTask = getNextDisplayTaskLoPri(nextTaskLoPri);
  }

  if (lastTask == newTask) {
    return;
  }
  if (lastTask) {
    SoftTimer.remove(lastTask);
  }
  SoftTimer.add(newTask);
  lastTask = newTask;
}

void displayTriggerEffects() {
#ifdef EFFECT_ENABLED
  bool hasEffects = false;
  for (byte i = 0; i < 6; i++) {
    if (dataToDisplayOld[i] != dataToDisplay[i]) {
      dataToDisplayOld[i] = dataToDisplay[i];
      dataIsTransitioning[i] = EFFECT_SPEED;
      hasEffects = true;
    }
  }

  if (hasEffects) {
    SoftTimer.add(&T_displayEffectsUpdate);
  }
#endif
}

#ifdef EFFECT_ENABLED
void displayEffectsUpdate(Task *me) {
  const unsigned long milliDelta = (me->nowMicros - me->lastCallTimeMicros) / 1000UL;
  bool hadEffects = false;
  for (byte i = 0; i < 6; i++) {
    if (dataIsTransitioning[i] > milliDelta) {
      dataIsTransitioning[i] -= milliDelta;
      hadEffects = true;
    } else {
      dataIsTransitioning[i] = 0;
    }
  }
  if (!hadEffects) {
    SoftTimer.remove(me);
  }
}
#endif

void updateDisplayCountdown(Task *me) {
  if (countdownTo < millis()) {
    const uint16_t sym = (second() % 2) ? NO_TUBES : getNumber(0);
    for (byte i = 0; i < 6; i++) {
      dataToDisplay[i] = sym;
    }
    return;
  }
  if (showShortTime(countdownTo - millis(), true)) {
    displayTriggerEffects();
  }
}

void updateDisplayStopwatch(Task *me) {
  if (stopwatchRunning) {
    stopwatchTime += (me->nowMicros - me->lastCallTimeMicros) / 1000UL;
  }

  if (showShortTime(stopwatchTime, true)) {
    displayTriggerEffects();
  }
}

void updateDisplayAntiPoison(Task *me) {
  if (antiPoisonEnd <= millis()) {
    SoftTimer.remove(me);
    changeUpdater(NULL);
    return;
  }
  const uint16_t sym = getNumber((antiPoisonEnd - millis()) / ANTI_POISON_DELAY);
  for (byte i = 0; i < 6; i++) {
    dataToDisplay[i] = sym;
    dataIsTransitioning[i] = 0;
  }
}

void updateDisplayClock(Task *me) {
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

  if (h < 4 && s % 5 == 2) {
    displayAntiPoison(1);
    return;
  }

  displayTriggerEffects();
}

void updateColors(Task *me) {
  float factor = 1.0;
  if (millis() < holdColorEaseInTarget) {
    factor = 1.0 - ((float)(holdColorEaseInTarget - millis()) / (float)(holdColorEaseInTarget - holdColorStartTime));
  } else if (millis() > holdColorEaseOutTarget) {
    SoftTimer.remove(me);
    factor = 0.0;
  } else if (millis() > holdColorSteadyTarget) {
    factor = (float)(holdColorEaseOutTarget - millis()) / (float)(holdColorEaseOutTarget - holdColorSteadyTarget);
  }
  analogWrite(PIN_LED_RED, setR * factor);
  analogWrite(PIN_LED_GREEN, setG * factor);
  analogWrite(PIN_LED_BLUE, setB * factor);
}

/*********************/
/* UTILITY FUNCTIONS */
/*********************/
#define hexCharToNum(c) ((c <= '9') ? c - '0' : c - '7')
byte hexInputToByte(const byte offset) {
  const byte msn = inputString[offset];
  const byte lsn = inputString[offset + 1];
  return (hexCharToNum(msn) << 4) + hexCharToNum(lsn);
}

uint16_t getNumber(const byte idx) {
  return 1 << (idx % 10);
}

void setDots(const bool upper, const bool lower) {
  dotMask = (upper ? 0 : MASK_UPPER_DOTS) | (lower ? 0 : MASK_LOWER_DOTS);
}

void noColor() {
  analogWrite(PIN_LED_RED, 0);
  analogWrite(PIN_LED_GREEN, 0);
  analogWrite(PIN_LED_BLUE, 0);
  SoftTimer.remove(&T_updateColors);
}

void displayAntiPoison(const unsigned long count) {
  antiPoisonEnd = millis() + (ANTI_POISON_DELAY * 10UL * count);
  changeUpdater(NULL);
  SoftTimer.add(&T_updateDisplayAntiPoison);
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

void renderNixies(Task *me) {
  static byte anodeGroup = 0;

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

