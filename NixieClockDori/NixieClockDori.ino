#include <SPI.h>
#include <TimeLib.h>
#include <SoftTimer.h>
#include <MemoryUsage.h>

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

void renderNixies(Task *me);
void cycleDisplayUpdater(Task *me);
void serialReader(Task *me);

DisplayTask_Clock displayClock;
DisplayTask_Stopwatch displayStopwatch;
DisplayTask_Countdown displayCountdown;
DisplayTask_Flash displayFlash;

Task T_cycleDisplayUpdater(5000, cycleDisplayUpdater);
Task T_serialReader(0, serialReader);

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

	// Begin initialization routines
	serialInit();
	rtcInit();
	displayInit();

	randomSeed(analogRead(A3) + now());

	SoftTimer.add(&T_serialReader);
	SoftTimer.add(&T_cycleDisplayUpdater);

	displayClock.loPri = true;
	displayClock.add();

	cycleDisplayUpdater(NULL);

	digitalWrite(PIN_HIGH_VOLTAGE_ENABLE, HIGH);

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
			displayCountdown.to = 0;
			displayStopwatch.reset();
			if (!DisplayTask::current->canShow()) {
				cycleDisplayUpdater(NULL);
				T_cycleDisplayUpdater.lastCallTimeMicros = micros();
			}
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
			// F [MMMMMMMM D NNNNNN [RR GG BB]]
			// M = milliseconds (Dec), D = dots (Bitmask Dec) to show the message, N = Nixie message (Dec), R = Red (Hex), G = Green (Hex), B = Blue (Hex)
			// Shows a "flash"/"alert" message on the clock (will show this message instead of the time for <M> milliseconds. Does not use/reset hold when 0). Dots are bit 1 for lower and bit 2 for upper. Turned off when HIGH
			// If sent without any parameters, resets current flash message and goes back to clock mode
			// ^F0000100021337NA|-15360
		case 'F':
			if (inputString.length() < 16) {
				if (inputString.length() < 3) { // Allow for \r\n
					cycleDisplayUpdater(NULL);
					serialSend(F("F OK"));
					break;
				}
				serialSend(F("F BAD (Invalid length; expected 16 or 1)"));
				break;
			}

			displayFlash.endTime = millis() + (unsigned long)inputString.substring(1, 9).toInt();

			tmpData = inputString[9] - '0';
			displayFlash.dotsMask = makeDotMask((tmpData & 2) == 2, (tmpData & 1) == 1);
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

			serialSend(F("F OK"));
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
			serialSend(F("C OK"));
			break;
			// W C [RR GG BB]
			// C = subcommand, R = Red (Hex), G = Green (Hex), B = Blue (Hex)
			// Controls the stopwatch. R for reset/disable, P for pause, U for un-pause, S for start/restart
			// ^WS|-8015
			// ^WR|-3952
		case 'W':
			if (inputString.length() < 2) {
				serialSend(F("W BAD (Invalid length; expected 2)"));
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
				Serial.print(F("W BAD (Invalid C)"));
				break;
			}
			if (tmpData) {
				showIfPossibleOtherwiseRotateIfCurrent(&displayStopwatch);
				serialSend(F("W OK"));
			}
			break;
			// ^D|-5712
		case 'D':
			serialSend("D OK " + String(me->nowMicros - me->lastCallTimeMicros) + " " + String(mu_freeRam()));
			break;
		}
	}
}

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
		DisplayTask::current = displayTask;
	}
	else if (displayTask == DisplayTask::current) {
		cycleDisplayUpdater(NULL);
	}
	else {
		return;
	}
	T_cycleDisplayUpdater.lastCallTimeMicros = micros();
}

void cycleDisplayUpdater(Task *me) {
	DisplayTask::current = DisplayTask::findNextValid(DisplayTask::current);
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
