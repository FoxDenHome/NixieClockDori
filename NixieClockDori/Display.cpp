#include "Display.h"
#include "DisplayTask.h"
#include "DisplayDriver.h"
#include "const.h"
#include "config.h"

#if !defined(DISPLAY_NEEDS_TIMER1)
#define autoAnalogWrite analogWrite
#elif defined(TIMER1_C_PIN)
#include <TimerOne.h>
#define autoAnalogWrite(PIN, VALUE) \
	if ((PIN) == TIMER1_A_PIN || (PIN) == TIMER1_B_PIN || (PIN) == TIMER1_C_PIN) { \
		Timer1.pwm(PIN, (VALUE) << 2); \
	} \
	else { \
		analogWrite(PIN, VALUE); \
	}
#else
#include <TimerOne.h>
#define autoAnalogWrite(PIN, VALUE) \
	if ((PIN) == TIMER1_A_PIN || (PIN) == TIMER1_B_PIN) { \
		Timer1.pwm(PIN, (VALUE) << 2); \
	} \
	else { \
		analogWrite(PIN, VALUE); \
	}
#endif

const byte MASK_UPPER_DOTS = 1;
const byte MASK_LOWER_DOTS = 2;
const byte MASK_BOTH_DOTS = MASK_UPPER_DOTS | MASK_LOWER_DOTS;

unsigned long antiPoisonEnd = 0;
unsigned long nextDisplayRender = 0;

uint16_t displayDataA[6] = { NO_TUBES, NO_TUBES, NO_TUBES, NO_TUBES, NO_TUBES, NO_TUBES };
uint16_t displayDataB[6] = { NO_TUBES, NO_TUBES, NO_TUBES, NO_TUBES, NO_TUBES, NO_TUBES };
uint16_t* displayDataBack = displayDataA;
uint16_t* displayDataFront = displayDataB;

byte dotMask = 0;
bool doFlip = true;

DisplayEffect currentEffect = SLOT_MACHINE;

void displayAntiPoisonOff() {
	antiPoisonEnd = 0;
}

void displayAntiPoison(const unsigned long count) {
	const unsigned long newEnd = millis() + (ANTI_POISON_DELAY * 10UL * count);
	if (newEnd > antiPoisonEnd) {
		antiPoisonEnd = newEnd;
	}
}

uint16_t getNumber(const byte idx) {
	return 1 << (idx % 10);
}

byte makeDotMask(const bool upper, const bool lower) {
	return (upper ? 0 : MASK_UPPER_DOTS) | (lower ? 0 : MASK_LOWER_DOTS);
}

bool showShortTime(const unsigned long timeMs, bool trimLZ, uint16_t dataToDisplay[], bool alwaysLong) {
	if (timeMs >= ONE_HOUR_IN_MS || alwaysLong) { // Show H/M/S
		trimLZ = insert2(0, (timeMs / ONE_HOUR_IN_MS) % 100, trimLZ, dataToDisplay);
		trimLZ = insert2(2, (timeMs / ONE_MINUTE_IN_MS) % 60, trimLZ, dataToDisplay);
		insert2(4, (timeMs / ONE_SECOND_IN_MS) % 60, trimLZ, dataToDisplay);
		return true;
	}
	else { // Show M/S/MS
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
	return data == 0 && trimLeadingZero;
}

unsigned long dataIsTransitioning[6] = { 0, 0, 0, 0, 0, 0 };
uint16_t dataToDisplayPrevious[6] = { NO_TUBES, NO_TUBES, NO_TUBES, NO_TUBES, NO_TUBES, NO_TUBES };
bool renderAlways = false;

void renderNixies(const unsigned long curMicros, const unsigned long microDelta) {
	static byte oldAntiPoisonIdx = 255;
	static uint16_t antiPoisonTable[6];

	bool allowEffects = false;

	static byte redOld, greenOld, blueOld;

	static uint16_t dataToDisplayOld[6] = { NO_TUBES, NO_TUBES, NO_TUBES, NO_TUBES, NO_TUBES, NO_TUBES };
	static byte redPrevious, greenPrevious, bluePrevious;
	static long colorTransProg;

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
				displayDataBack[i] = randNbr;
				dataIsTransitioning[i] = 0;
			}
			doFlip = true;
			oldAntiPoisonIdx = idx;
		}
		else {
			doFlip = false;
		}
	}
	else if (DisplayTask::current) {
		allowEffects = DisplayTask::current->refresh(displayDataBack);

		if (currentEffect != NONE) {
			byte redNow = redOld, greenNow = greenOld, blueNow = blueOld;

			if (colorTransProg > 0 && allowEffects && colorTransProg > (long)microDelta) {
				colorTransProg -= microDelta;
				redNow = redOld + (((redPrevious - redOld) * colorTransProg) / EFFECT_SPEED);
				greenNow = greenOld + (((greenPrevious - greenOld) * colorTransProg) / EFFECT_SPEED);
				blueNow = blueOld + (((bluePrevious - blueOld) * colorTransProg) / EFFECT_SPEED);
				autoAnalogWrite(PIN_LED_RED, redNow);
				autoAnalogWrite(PIN_LED_GREEN, greenNow);
				autoAnalogWrite(PIN_LED_BLUE, blueNow);
			}
			else if (colorTransProg >= 0) {
				colorTransProg = -1;
				autoAnalogWrite(PIN_LED_RED, redNow);
				autoAnalogWrite(PIN_LED_GREEN, greenNow);
				autoAnalogWrite(PIN_LED_BLUE, blueNow);
			}

			if (DisplayTask::current->red != redOld || DisplayTask::current->green != greenOld || DisplayTask::current->blue != blueOld) {
				redPrevious = redNow;
				redOld = DisplayTask::current->red;
				greenPrevious = greenNow;
				greenOld = DisplayTask::current->green;
				bluePrevious = blueNow;
				blueOld = DisplayTask::current->blue;
				colorTransProg = EFFECT_SPEED;
			}
		}
		else {
			if (DisplayTask::current->red != redOld) {
				redOld = DisplayTask::current->red;
				autoAnalogWrite(PIN_LED_RED, redOld);
			}
			if (DisplayTask::current->green != greenOld) {
				greenOld = DisplayTask::current->green;
				autoAnalogWrite(PIN_LED_GREEN, greenOld);
			}
			if (DisplayTask::current->blue != blueOld) {
				blueOld = DisplayTask::current->blue;
				autoAnalogWrite(PIN_LED_BLUE, blueOld);
			}
		}

		dotMask = DisplayTask::current->dotMask;
	}

	// Progress through effect
	const  bool effectsOn = allowEffects && currentEffect != NONE;

	bool hasEffects = false, setFlip = false;
	for (byte i = 0; i < 6; i++) {
		const uint16_t cur = displayDataBack[i];
		if (dataToDisplayOld[i] != cur) {
			dataToDisplayPrevious[i] = dataToDisplayOld[i];
			dataToDisplayOld[i] = cur;
			if (effectsOn) {
				dataIsTransitioning[i] = EFFECT_SPEED;
			}
			setFlip = true;
		}

		if (!effectsOn) {
			continue;
		}

		const unsigned long tubeTrans = dataIsTransitioning[i];

		if (tubeTrans > microDelta) {
			if (currentEffect == SLOT_MACHINE) {
				displayDataBack[i] = getNumber(tubeTrans / (EFFECT_SPEED / 10UL));
			}
			dataIsTransitioning[i] -= microDelta;
			hasEffects = true;
			setFlip = true;
		}
		else if (tubeTrans > 0) {
			displayDataBack[i] = dataToDisplayOld[i];
			dataIsTransitioning[i] = 0;
			setFlip = true;
		}
	}

	if (effectsOn) {
		if (currentEffect == TRANSITION) {
			renderAlways = hasEffects;
		}
	}

	doFlip = setFlip;
}

void displayInit() {

}

void displayLoop(const unsigned long curMicros) {
	static unsigned long lastRenderTime = 0;

	if (nextDisplayRender <= curMicros) {
		renderNixies(curMicros, curMicros - lastRenderTime);
		lastRenderTime = curMicros;
		nextDisplayRender = curMicros + (DISPLAY_RENDER_STEP * 33UL);
	}
}
