#include <SPI.h>
#include <TimeLib.h>
#include <MemoryUsage.h>
#include <OneButton.h>

#include "rtc.h"
#include "config.h"
#include "const.h"
#include "crcserial.h"
#include "Display.h"
#include "DisplayTask.h"

#include "DisplayTask_Clock.h"
#include "DisplayTask_Stopwatch.h"
#include "DisplayTask_Countdown.h"
#include "DisplayTask_Flash.h"

/****************/
/* PROGRAM CODE */
/****************/

/******************/
/* TASK VARIABLES */
/******************/

DisplayTask_Clock displayClock;
DisplayTask_Stopwatch displayStopwatch;
DisplayTask_Countdown displayCountdown;
DisplayTask_Flash displayFlash;

/**************************/
/* ARDUINO EVENT HANDLERS */
/**************************/

#define _DECL_BUTTON_FN(NAME, FUNC) \
	void __ ## NAME ## _BUTTON_ ## FUNC () { \
		 if (DisplayTask::current) { \
			DisplayTask::current->handleButtonPress(NAME, FUNC); \
		 } \
	}

#define _SETUP_BUTTON_FN(NAME, FUNC) \
	NAME ## Button.attach ## FUNC (__ ## NAME ## _BUTTON_ ## FUNC);

#define DECL_BUTTON(NAME) \
	OneButton NAME ## Button(PIN_BUTTON_ ## NAME, true); \
	_DECL_BUTTON_FN(NAME, Click) \
	_DECL_BUTTON_FN(NAME, DoubleClick) \
	_DECL_BUTTON_FN(NAME, LongPressStart)

#define SETUP_BUTTON(NAME) \
	_SETUP_BUTTON_FN(NAME, Click) \
	_SETUP_BUTTON_FN(NAME, DoubleClick) \
	_SETUP_BUTTON_FN(NAME, LongPressStart)

DECL_BUTTON(DOWN)
DECL_BUTTON(UP)
DECL_BUTTON(SET)

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

	randomSeed(analogRead(A3) + now());

	displayClock.loPri = true;
	displayClock.add();

	displayStopwatch.add();
	displayCountdown.add();

	DisplayTask::cycleDisplayUpdater();

	SETUP_BUTTON(UP);
	SETUP_BUTTON(DOWN);
	SETUP_BUTTON(SET);

	digitalWrite(PIN_HIGH_VOLTAGE_ENABLE, HIGH);

	serialSendF("< Ready");
}

void loop() {
	UPButton.tick();
	DOWNButton.tick();
	SETButton.tick();

	const unsigned long curMicros = micros();
	displayLoop(curMicros);
	if (DisplayTask::nextDisplayCycleMicros <= curMicros) {
		DisplayTask::cycleDisplayUpdater();
	}
	serialPoll();
}

void serialPoll() {
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
				serialSendF("T BAD (Invalid length; expected 16)");
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
			serialSendF("T OK");
			break;
			// X
			// Performs a display reset of all modes
		case 'X':
			displayCountdown.to = 0;
			displayStopwatch.reset();
			displayFlash.endTime = 0;
			if (!DisplayTask::current->canShow()) {
				DisplayTask::cycleDisplayUpdater();
			}
			serialSendF("X OK");
			break;
			// P CC
			// C = Count (Dec)
			// Performs an anti poisoning routine <C> times
			// ^P01|-20043
		case 'P':
			if (inputString.length() < 2) {
				serialSendF("P BAD (Invalid length; expected 2)");
				break;
			}
			displayAntiPoison(inputString.substring(1, 3).toInt());
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

			displayFlash.endTime = millis() + (unsigned long)inputString.substring(1, 9).toInt();

			tmpData = inputString[9] - '0';
			displayFlash.dotMask = makeDotMask((tmpData & 2) == 2, (tmpData & 1) == 1);
			for (byte i = 0; i < 6; i++) {
				tmpData = inputString[i + 10];
				if (tmpData == 'N') {
					displayFlash.symbols[i] = NO_TUBES;
				}
				else if (tmpData == 'A') {
					displayFlash.symbols[i] = ALL_TUBES;
				}
				else {
					displayFlash.symbols[i] = getNumber(tmpData - '0');
				}
			}

			setColorFromInput(&displayFlash, 16);
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
				displayCountdown.to = 0;
			}
			else {
				displayCountdown.to = millis() + inputString.substring(1, 9).toInt();
			}
			setColorFromInput(&displayCountdown, 9);
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
			setColorFromInput(&displayStopwatch, 2);
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

void setColorFromInput(DisplayTask *displayTask, const byte offset) {
	if (inputString.length() < offset + 6) {
		return;
	}
	displayTask->red = hexInputToByte(offset);
	displayTask->green = hexInputToByte(offset + 2);
	displayTask->blue = hexInputToByte(offset + 4);
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
}

#define hexCharToNum(c) ((c <= '9') ? c - '0' : c - '7')
byte hexInputToByte(const byte offset) {
	const byte msn = inputString[offset];
	const byte lsn = inputString[offset + 1];
	return (hexCharToNum(msn) << 4) + hexCharToNum(lsn);
}
