#include <SPI.h>
#include <EEPROM.h>
#include <Wire.h>
#include <TimeLib.h>
#include <DS3232RTC.h>
#include <MemoryUsage.h>
#include <OneButton.h>
#include <FastCRC.h>

#include <avr/wdt.h>

#include "rtc.h"
#include "config.h"
#include "const.h"
#include "crcserial.h"
#include "Display.h"
#include "DisplayDriver.h"
#include "DisplayTask.h"

#include "DisplayTask_Clock.h"
#include "DisplayTask_Date.h"
#include "DisplayTask_Stopwatch.h"
#include "DisplayTask_Countdown.h"
#include "DisplayTask_Flash.h"
#include "DisplayTask_Temperature.h"

#ifdef DISPLAY_NEEDS_TIMER1
#include <TimerOne.h>
#endif

/****************/
/* PROGRAM CODE */
/****************/

/******************/
/* TASK VARIABLES */
/******************/

DisplayTask_Clock displayClock;
DisplayTask_Date displayDate;
DisplayTask_Stopwatch displayStopwatch;
DisplayTask_Countdown displayCountdown;
DisplayTask_Flash displayFlash;
DisplayTask_Temperature displayTemp;

/**************************/
/* ARDUINO EVENT HANDLERS */
/**************************/

void(*resetFunc) (void) = 0;

#define _DECL_BUTTON_FN(NAME, FUNC) \
	void __ ## NAME ## _BUTTON_ ## FUNC () { \
		DisplayTask::buttonHandler(NAME, FUNC); \
	}

#define _SETUP_BUTTON_FN(NAME, FUNC) \
	NAME ## Button.attach ## FUNC (__ ## NAME ## _BUTTON_ ## FUNC);

#define DECL_BUTTON(NAME) \
	OneButton NAME ## Button(PIN_BUTTON_ ## NAME, true); \
	_DECL_BUTTON_FN(NAME, Click) \
	_DECL_BUTTON_FN(NAME, LongPressStart)

#define SETUP_BUTTON(NAME) \
	NAME ## Button.setClickTicks(200); \
	NAME ## Button.setPressTicks(500); \
	_SETUP_BUTTON_FN(NAME, Click) \
	_SETUP_BUTTON_FN(NAME, LongPressStart)

DECL_BUTTON(DOWN)
DECL_BUTTON(UP)
DECL_BUTTON(SET)

void setup() {
	const uint8_t mcusr_mirror = MCUSR;
	MCUSR = 0;
	wdt_disable();

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
	digitalWrite(PIN_DISPLAY_LATCH, LOW);
	pinMode(PIN_HIZ, OUTPUT);
	digitalWrite(PIN_HIZ, LOW);

	pinMode(PIN_BUTTON_SET, INPUT_PULLUP);
	pinMode(PIN_BUTTON_UP, INPUT_PULLUP);
	pinMode(PIN_BUTTON_DOWN, INPUT_PULLUP);

	// Begin initialization routines
	serialInit();
	rtcInit();
	displayInit();
	displayDriverInit();

	randomSeed(analogRead(A4) + now());

	DisplayEffect loadEffect;
	EEPROM.get(EEPROM_STORAGE_CURRENT_EFFECT, loadEffect);
	currentEffect = loadEffect;

	displayClock.loPri = true;
	displayClock.add();
	displayDate.loPri = true;
	displayDate.add();

	displayStopwatch.add();
	displayCountdown.add();

	displayTemp.loPri = true;
	displayTemp.add();

	displayClock.loadColor(EEPROM_STORAGE_CLOCK_RGB);
	displayDate.loadColor(EEPROM_STORAGE_DATE_RGB);
	displayDate.loadConfig(EEPROM_STORAGE_DATE_AUTO);
	displayStopwatch.loadColor(EEPROM_STORAGE_STOPWATCH_RGB);
	displayCountdown.loadColor(EEPROM_STORAGE_COUNTDOWN_RGB);
	displayCountdown.loadConfig(EEPROM_STORAGE_COUNTDOWN);
	displayTemp.loadColor(EEPROM_STORAGE_TEMPERATURE_RGB);

	DisplayTask::cycleDisplayUpdater();

	SETUP_BUTTON(UP);
	SETUP_BUTTON(DOWN);
	SETUP_BUTTON(SET);

	serialSend2(F("< Ready "), String(mcusr_mirror));

	wdt_enable(WDTO_250MS);

	digitalWrite(PIN_HIGH_VOLTAGE_ENABLE, HIGH);
}

void loop() {
	wdt_reset();

	UPButton.tick();
	DOWNButton.tick();
	SETButton.tick();

	if (DisplayTask::nextDisplayCycleMicros <= micros()) {
		DisplayTask::cycleDisplayUpdater();
	}
	displayLoop();
	displayDriverLoop();

	serialPoll();
}

void serialPoll() {
	while (Serial.available()) {
		if (!serialReadNext()) {
			continue;
		}

		byte tmpData;
		const unsigned long curMillis = millis();

		switch (inputString[0]) {
			// T HH II SS DD MM YY
			// H = Hours, I = Minutes, S = Seconds, D = Day of month, M = month, Y = year (ALL Dec)
			// Sets the time on the clock
			// ^T175630010318|17199
		case 'T':
			if (inputString.length() < 13) {
				serialSendF("T BAD (Invalid length; expected 13)");
				break;
			}
			tmElements_t tm;
			tm.Hour = inputString.substring(1, 3).toInt();
			tm.Minute = inputString.substring(3, 5).toInt();
			tm.Second = inputString.substring(5, 7).toInt();
			tm.Day = inputString.substring(6, 9).toInt();
			tm.Month = inputString.substring(9, 11).toInt();
			tm.Year = y2kYearToTm(inputString.substring(11, 13).toInt());
			rtcSetTime(tm);
			serialSendF("T OK");
			break;
			// H
			// Pings the display ("Hello")
			// ^H|10300
		case 'H':
			serialSendF("H OK " FW_VERSION);
			break;
			// X
			// Performs a display reset of all modes
			// ^X|14861
		case 'X':
			MCUSR = 0;
			wdt_disable();

			for (uint16_t i = 0; i < EEPROM.length(); i++) {
				EEPROM.write(i, 0);
			}
			serialSendF("X OK");

			// Get the watchdog stuck to force a reset!
			wdt_enable(WDTO_15MS);
			while (1) {
				delay(10);
			}

			break;
			// P [CC]
			// C = Count (Dec)
			// Performs an anti poisoning routine <C> times
			// ^P01|-20043
			// ^P|-17659
		case 'P':
			if (inputString.length() < 3) {
				displayAntiPoisonOff();
			}
			else {
				displayAntiPoison(inputString.substring(1, 3).toInt());
			}
			serialSendF("P OK");
			break;
			// F [MMMMMMMM D NNNNNN [RR GG BB]]
			// M = milliseconds (Dec), D = dots (Bitmask Dec) to show the message, N = Nixie message (Dec), R = Red (Hex), G = Green (Hex), B = Blue (Hex)
			// Shows a "flash"/"alert" message on the clock (will show this message instead of the time for <M> milliseconds. Does not use/reset hold when 0). Dots are bit 1 for lower and bit 2 for upper. Turned off when HIGH
			// If sent without any parameters, resets current flash message and goes back to clock mode
			// ^F0000100021337NA|-15360
		case 'F':
			if (inputString.length() < 16) {
				if (inputString.length() < 3) { // Allow for \r\n
					DisplayTask::cycleDisplayUpdater();
					serialSendF("F OK");
					break;
				}
				serialSendF("F BAD (Invalid length; expected 16 or 1)");
				break;
			}

			displayFlash.allowEffects = (curMillis - displayFlash.lastUpdate) >= 900;
			displayFlash.lastUpdate = curMillis;
			displayFlash.endTime = curMillis + (unsigned long)inputString.substring(1, 9).toInt();

			tmpData = inputString[9] - '0';
			displayFlash.dotMask = makeDotMask((tmpData & 2) == 2, (tmpData & 1) == 1);
			for (byte j = 0; j < 3; j++) {
				displayFlash.symbols[j] = 0;
			}
			for (byte i = 0; i < 6; i++) {
				tmpData = inputString[i + 10];
				const byte j = i >> 1;
				const byte n = ((i & 1) == 1) ? 4 : 0;
				if (tmpData == 'N') {
					displayFlash.symbols[j] |= NO_TUBES << n;
				}
				else if (tmpData == 'A') {
					displayFlash.symbols[j] |= ALL_TUBES << n;
				}
				else {
					displayFlash.symbols[j] |= getNumber(tmpData - '0') << n;
				}
			}

			setColorFromInput(&displayFlash, 16, -1);
			showIfPossibleOtherwiseRotateIfCurrent(&displayFlash);

			serialSendF("F OK");
			break;
			// C [MMMMMMMM [RR GG BB]]
			// M = Time in ms (Dec), R = Red (Hex), G = Green (Hex), B = Blue (Hex)
			// Starts a countdown for <M> ms. Stops countdown if <M> = 0
			// ^C00010000|9735
			// ^C|-26281
		case 'C':
			if (inputString.length() < 9) {
				displayCountdown.reset();
			}
			else {
				displayCountdown.timeReset = inputString.substring(1, 9).toInt();
				displayCountdown.start();
			}
			setColorFromInput(&displayCountdown, 9, EEPROM_STORAGE_COUNTDOWN_RGB);
			showIfPossibleOtherwiseRotateIfCurrent(&displayCountdown);
			serialSendF("C OK");
			break;
			// W C [RR GG BB]
			// C = subcommand, R = Red (Hex), G = Green (Hex), B = Blue (Hex)
			// Controls the stopwatch. R for reset/disable, P for pause, U for un-pause, S for start/restart
			// ^WS|-8015
			// ^WR|-3952
		case 'W':
			if (inputString.length() < 2) {
				serialSendF("W BAD (Invalid length; expected 2)");
				break;
			}
			setColorFromInput(&displayStopwatch, 2, EEPROM_STORAGE_STOPWATCH_RGB);
			tmpData = true;
			switch (inputString[1]) {
			case 'R':
				displayStopwatch.reset();
				break;
			case 'P':
				displayStopwatch.pause();
				break;
			case 'U':
				displayStopwatch.resume();
				break;
			case 'S':
				displayStopwatch.start();
				break;
			default:
				tmpData = false;
				serialSendF("W BAD (Invalid C)");
				break;
			}
			if (tmpData) {
				showIfPossibleOtherwiseRotateIfCurrent(&displayStopwatch);
				serialSendF("W OK");
			}
			break;
			// ^E0|-9883
			// ^E1|-14012
			// ^E2|-1753
		case 'E':
			if (inputString.length() < 2) {
				serialSendF("E BAD (Invalid length; expected 2)");
				break;
			}
			currentEffect = (DisplayEffect)(inputString[1] - '0');
			serialSendF("E OK");
			break;
			// ^D|-5712
		case 'D':
			serialSend2(F("D OK "), String(mu_freeRam()));
			break;
		}
	}
}


/*********************/
/* UTILITY FUNCTIONS */
/*********************/

void setColorFromInput(DisplayTask *displayTask, const byte offset, const int16_t eepromBase) {
	if (inputString.length() < (unsigned int)offset + 6) {
		return;
	}
	displayTask->red = hexInputToByte(offset);
	displayTask->green = hexInputToByte(offset + 2);
	displayTask->blue = hexInputToByte(offset + 4);
	displayTask->saveColor(eepromBase);
}

void showIfPossibleOtherwiseRotateIfCurrent(DisplayTask *displayTask) {
	if (displayTask->canShow()) {
		displayTask->add();
		DisplayTask::editMode = false;
		DisplayTask::current = displayTask;
		DisplayTask::nextDisplayCycleMicros = micros() + DISPLAY_CYCLE_PERIOD;
	}
	else if (displayTask == DisplayTask::current) {
		DisplayTask::cycleDisplayUpdater();
	}
	else {
		return;
	}
	displayAntiPoisonOff();
}

#define hexCharToNum(c) ((c <= '9') ? c - '0' : c - '7')
byte hexInputToByte(const byte offset) {
	const byte msn = inputString[offset];
	const byte lsn = inputString[offset + 1];
	return (hexCharToNum(msn) << 4) + hexCharToNum(lsn);
}
