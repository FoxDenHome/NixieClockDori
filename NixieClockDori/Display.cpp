#include "Display.h"
#include "DisplayTask.h"
#include "const.h"
#include "config.h"
#include <SPI.h>

const byte MASK_UPPER_DOTS = 1;
const byte MASK_LOWER_DOTS = 2;
const byte MASK_BOTH_DOTS = MASK_UPPER_DOTS | MASK_LOWER_DOTS;

unsigned long antiPoisonEnd = 0;
unsigned long nextDisplayRender = 0;

uint16_t displayData[6] = { NO_TUBES, NO_TUBES, NO_TUBES, NO_TUBES, NO_TUBES, NO_TUBES };
byte dotMask = 0;
byte anodeGroup = 9;

#ifdef DISPLAY_BLANK_PERIOD
boolean blankNext = true;
#endif

void displayAntiPoison(const unsigned long count) {
	antiPoisonEnd = millis() + (ANTI_POISON_DELAY * 10UL * count);
}

uint16_t getNumber(const byte idx) {
	return 1 << (idx % 10);
}

byte makeDotMask(const bool upper, const bool lower) {
	return (upper ? 0 : MASK_UPPER_DOTS) | (lower ? 0 : MASK_LOWER_DOTS);
}

bool showShortTime(const unsigned long timeMs, bool trimLZ, uint16_t dataToDisplay[], byte *dotMask) {
	if (timeMs >= ONE_HOUR_IN_MS) { // Show H/M/S
		*dotMask = makeDotMask(true, false);
		trimLZ = insert2(0, (timeMs / ONE_HOUR_IN_MS) % 100, trimLZ, dataToDisplay);
		trimLZ = insert2(2, (timeMs / ONE_MINUTE_IN_MS) % 60, trimLZ, dataToDisplay);
		insert2(4, (timeMs / ONE_SECOND_IN_MS) % 60, trimLZ, dataToDisplay);
		return true;
	}
	else { // Show M/S/MS
		*dotMask = makeDotMask(false, true);
		trimLZ = insert2(0, (timeMs / ONE_MINUTE_IN_MS) % 60, trimLZ, dataToDisplay);
		trimLZ = insert2(2, (timeMs / ONE_SECOND_IN_MS) % 60, trimLZ, dataToDisplay);
		insert2(4, (timeMs / 10UL) % 100, trimLZ, dataToDisplay);
		return false; // Don't allow transition effects on rapid timer
	}
}

void insert1(const byte offset, const byte data, const bool trimLeadingZero, uint16_t dataToDisplay[]) {
	if (data == 0 && trimLeadingZero) {
		dataToDisplay[offset] = 0;
	}
	else {
		dataToDisplay[offset] = getNumber(data);
	}
}

bool insert2(const byte offset, const byte data, const bool trimLeadingZero, uint16_t dataToDisplay[]) {
	insert1(offset, data / 10, trimLeadingZero, dataToDisplay);
	insert1(offset + 1, data, trimLeadingZero, dataToDisplay);
	return data == 0;
}

void renderNixies(const unsigned long curMicros, const unsigned long microDelta) {
	static byte oldAntiPoisonIdx = 255;
	static uint16_t antiPoisonTable[6];

	static byte redOld, greenOld, blueOld;
#ifdef EFFECT_ENABLED
	static unsigned long dataIsTransitioning[6] = { 0, 0, 0, 0, 0, 0 };
	static boolean allTubesOld = true;
	static uint16_t dataToDisplayOld[6] = { NO_TUBES, NO_TUBES, NO_TUBES, NO_TUBES, NO_TUBES, NO_TUBES };
	static byte redPrevious, greenPrevious, bluePrevious;
	static long colorTransProg;
#endif

	const unsigned long curMillis = millis();

	if (antiPoisonEnd > curMillis) {
		const byte idx = ((antiPoisonEnd - curMillis) / ANTI_POISON_DELAY) % 10;
		if (idx != oldAntiPoisonIdx) {
			if (idx == 9) {
				// Regenerate table
				memset(antiPoisonTable, 0, sizeof(antiPoisonTable));
			}
			for (byte i = 0; i < 6; i++) {
				uint16_t randNbr = getNumber(random(0, 10));
				while ((antiPoisonTable[i] & randNbr) == randNbr) {
					randNbr <<= 1;
					if (randNbr > ALL_TUBES) {
						randNbr = 1;
					}
				}
				antiPoisonTable[i] |= randNbr;
				displayData[i] = randNbr;
			}
			oldAntiPoisonIdx = idx;
		}
	}
	else if (DisplayTask::current) {
		const boolean doRender = DisplayTask::current->nextRender <= curMicros;

		boolean allowEffects = true;
		if (doRender) {
			DisplayTask::current->nextRender = curMicros + DisplayTask::current->renderPeriodMicros;
			allowEffects = DisplayTask::current->render();
		}

#ifdef EFFECT_ENABLED
		if (allowEffects) {
			// Start necessary effects (when display changed)
			if (doRender) {
				for (byte i = 0; i < 6; i++) {
					const uint16_t cur = DisplayTask::current->dataToDisplay[i];
					if (dataToDisplayOld[i] != cur || allTubesOld) {
						dataToDisplayOld[i] = cur;
						dataIsTransitioning[i] = EFFECT_SPEED;
					}
				}
			}

			for (byte i = 0; i < 6; i++) {
				unsigned long tubeTrans = dataIsTransitioning[i];
				if (tubeTrans > 0) {
#ifdef EFFECT_SLOT_MACHINE
					displayData[i] = getNumber(tubeTrans / (EFFECT_SPEED / 10UL));
#endif
				}
				else {
					displayData[i] = DisplayTask::current->dataToDisplay[i];
				}
			}

			allTubesOld = false;
		}
		else {
			allTubesOld = true;
		}

		byte redNow = redOld, greenNow = greenOld, blueNow = blueOld;

		if (colorTransProg > 0 && allowEffects && colorTransProg > microDelta) {
			colorTransProg -= microDelta;
			redNow = redOld + (((redPrevious - redOld) * colorTransProg) / EFFECT_SPEED);
			greenNow = greenOld + (((greenPrevious - greenOld) * colorTransProg) / EFFECT_SPEED);
			blueNow = blueOld + (((bluePrevious - blueOld) * colorTransProg) / EFFECT_SPEED);
			analogWrite(PIN_LED_RED, redNow);
			analogWrite(PIN_LED_GREEN, greenNow);
			analogWrite(PIN_LED_BLUE, blueNow);
		}
		else if (colorTransProg >= 0) {
			colorTransProg = -1;
			analogWrite(PIN_LED_RED, redNow);
			analogWrite(PIN_LED_GREEN, greenNow);
			analogWrite(PIN_LED_BLUE, blueNow);
		}

		if (DisplayTask::current->red != redOld) {
			redPrevious = redNow;
			redOld = DisplayTask::current->red;
			colorTransProg = EFFECT_SPEED;
		}
		if (DisplayTask::current->green != greenOld) {
			greenPrevious = greenNow;
			greenOld = DisplayTask::current->green;
			colorTransProg = EFFECT_SPEED;
		}
		if (DisplayTask::current->blue != blueOld) {
			bluePrevious = blueNow;
			blueOld = DisplayTask::current->blue;
			colorTransProg = EFFECT_SPEED;
		}

#else
		if (DisplayTask::current->red != redOld) {
			redOld = DisplayTask::current->red;
			analogWrite(PIN_LED_RED, redOld);
		}
		if (DisplayTask::current->green != greenOld) {
			greenOld = DisplayTask::current->green;
			analogWrite(PIN_LED_GREEN, greenOld);
		}
		if (DisplayTask::current->blue != blueOld) {
			blueOld = DisplayTask::current->blue;
			analogWrite(PIN_LED_BLUE, blueOld);
		}
#endif

		dotMask = DisplayTask::current->dotMask;
	}

	// Progress through effect
#ifdef EFFECT_ENABLED
	for (byte i = 0; i < 6; i++) {
		if (dataIsTransitioning[i] > microDelta) {
			dataIsTransitioning[i] -= microDelta;
		}
		else {
			dataIsTransitioning[i] = 0;
		}
	}
#endif
}

#ifdef DISPLAY_BLANK_PERIOD
void renderNixiesInt(bool blank) {
	const byte anodeControl = blank ? 0 : 1 << (anodeGroup + 4);
#else
void renderNixiesInt() {
	const byte anodeControl = 1 << (anodeGroup + 4);
#endif

	const byte curTubeL = anodeGroup << 1;
	const byte curTubeR = curTubeL + 1;

	const uint16_t tubeL = displayData[curTubeL];
	const uint16_t tubeR = displayData[curTubeR];

	digitalWrite(PIN_DISPLAY_LATCH, LOW);
	SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE2));
	SPI.transfer(dotMask);                            // [   ][   ][   ][   ][   ][   ][L1 ][L0 ] - L0     L1 - dots
	SPI.transfer(tubeR >> 6 | anodeControl);          // [   ][A2 ][A1 ][A0 ][RC9][RC8][RC7][RC6] - A0  -  A2 - anodes
	SPI.transfer(tubeR << 2 | tubeL >> 8);            // [RC5][RC4][RC3][RC2][RC1][RC0][LC9][LC8] - RC9 - RC0 - Right tubes cathodes
	SPI.transfer(tubeL);                              // [LC7][LC6][LC5][LC4][LC3][LC2][LC1][LC0] - LC9 - LC0 - Left tubes cathodes
	SPI.endTransaction();
	digitalWrite(PIN_DISPLAY_LATCH, HIGH);
}

void displayInit() {
	SPI.begin();
}

void displayLoop(const unsigned long curMicros) {
	static unsigned long lastRenderTime = 0;

	if (nextDisplayRender <= curMicros) {
#ifdef DISPLAY_BLANK_PERIOD
		renderNixiesInt(blankNext);
		if (blankNext) {
#else
		renderNixiesInt();
#endif
			if (++anodeGroup > 2) {
				anodeGroup = 0;
				renderNixies(curMicros, curMicros - lastRenderTime);
				lastRenderTime = curMicros;
			}
#ifdef DISPLAY_BLANK_PERIOD
		}
		blankNext = !blankNext;
		nextDisplayRender = curMicros + (blankNext ? DISPLAY_RENDER_PERIOD : DISPLAY_BLANK_PERIOD);
#else
		nextDisplayRender = curMicros + DISPLAY_RENDER_PERIOD;
#endif
	}
}
