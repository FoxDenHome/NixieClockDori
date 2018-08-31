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

unsigned long antiPoisonLeft = 0;
unsigned long lastDisplayRender = 0;

volatile byte displayData[3] = { NO_TUBES_BOTH, NO_TUBES_BOTH, NO_TUBES_BOTH };

volatile byte dataIsTransitioning[3] = { 0, 0, 0 };
volatile byte dataToDisplayPrevious[3] = { NO_TUBES_BOTH, NO_TUBES_BOTH, NO_TUBES_BOTH };
volatile bool renderAlways = false;
volatile bool renderNoMultiplex = false;

volatile byte dotMask = 0;

volatile DisplayEffect currentEffect = SLOT_MACHINE;

void displayAntiPoisonOff() {
	antiPoisonLeft = 0;
}

void displayAntiPoison(const unsigned long count) {
	if (count < 1) {
		return;
	}

	const unsigned long newLeft = (ANTI_POISON_DELAY * 10UL * count) - 1UL;
	if (newLeft > antiPoisonLeft) {
		antiPoisonLeft = newLeft;
	}
}

bool showShortTime(const unsigned long timeMs, bool trimLZ, bool alwaysLong) {
	if (timeMs >= ONE_HOUR_IN_MS || alwaysLong) { // Show H/M/S
		trimLZ = insert2(0, (timeMs / ONE_HOUR_IN_MS) % 100, trimLZ);
		trimLZ = insert2(2, (timeMs / ONE_MINUTE_IN_MS) % 60, trimLZ);
		insert2(4, (timeMs / ONE_SECOND_IN_MS) % 60, trimLZ);
		return true;
	}
	else { // Show M/S/MS
		trimLZ = insert2(0, (timeMs / ONE_MINUTE_IN_MS) % 60, trimLZ);
		trimLZ = insert2(2, (timeMs / ONE_SECOND_IN_MS) % 60, trimLZ);
		insert2(4, (timeMs / 10UL) % 100, trimLZ);
		return false; // Don't allow transition effects on rapid timer
	}
}

void insert1(const byte offset, const byte data, const bool trimLeadingZero) {
	const byte rOffset = offset >> 1;
	const byte rNibble = ((offset & 1) == 1) ? 4 : 0;

	const byte dData = displayData[rOffset] & ~(0xF << rNibble);
	if (data == 0 && trimLeadingZero) {
		displayData[rOffset] = dData | (NO_TUBES << rNibble);
	}
	else {
		displayData[rOffset] = dData | (getNumber(data) << rNibble);
	}
}

bool insert2(const byte offset, const byte data, const bool trimLeadingZero) {
	insert1(offset, data / 10, trimLeadingZero);
	insert1(offset + 1, data, trimLeadingZero);
	return data == 0 && trimLeadingZero;
}

void renderNixies() {
	static byte oldAntiPoisonIdx = 255;
	const uint16_t ALL_TUBES_ANTI_POISON = (1 << 10) - 1;
	static uint16_t antiPoisonTable[6] = { 0, 0, 0, 0, 0, 0 };

	bool allowEffects = false;

	static byte redOld, greenOld, blueOld;

	static byte dataToDisplayOld[3] = { NO_TUBES_BOTH, NO_TUBES_BOTH, NO_TUBES_BOTH };
	static byte redPrevious, greenPrevious, bluePrevious;
	static byte colorTransProg;

	const unsigned long curMillis = millis();
	static unsigned long lastCallMillis = 0;
	const unsigned long lastCallDelta = curMillis - lastCallMillis;

	if (antiPoisonLeft) {
		renderNoMultiplex = true;
		const byte idx = 9 - ((antiPoisonLeft / ANTI_POISON_DELAY) % 10);
		if (idx != oldAntiPoisonIdx) {
			byte curAP[6] = { 0, 0, 0, 0, 0, 0 };
			for (byte i = 0; i < 6; i++) {
				if (antiPoisonTable[i] == ALL_TUBES_ANTI_POISON) {
					antiPoisonTable[i] = 0;
				}
				byte randNbr = random(0, 10);
				while (antiPoisonTable[i] & (1 << randNbr)) {
					if (++randNbr > 9) {
						randNbr = 0;
					}
				}
				antiPoisonTable[i] |= (1 << randNbr);
				curAP[i] = randNbr;
			}
			for (byte i = 0; i < 3; i++) {
				const byte j = i << 1;
				displayData[i] = curAP[j] | (curAP[j + 1] << 4);
				dataIsTransitioning[i] = 0;
			}
			oldAntiPoisonIdx = idx;
		}
		if (lastCallDelta >= antiPoisonLeft) {
			antiPoisonLeft = 0;
		}
		else {
			antiPoisonLeft -= lastCallDelta;
		}
	}
	else {
		renderNoMultiplex = false;
		allowEffects = DisplayTask::current->refresh();

		if (currentEffect != NONE) {
			byte redNow = redOld, greenNow = greenOld, blueNow = blueOld;

			if (colorTransProg > 1 && allowEffects) {
				colorTransProg--;
				redNow = redOld + (((redPrevious - redOld) * colorTransProg) / EFFECT_SPEED);
				greenNow = greenOld + (((greenPrevious - greenOld) * colorTransProg) / EFFECT_SPEED);
				blueNow = blueOld + (((bluePrevious - blueOld) * colorTransProg) / EFFECT_SPEED);
				autoAnalogWrite(PIN_LED_RED, redNow);
				autoAnalogWrite(PIN_LED_GREEN, greenNow);
				autoAnalogWrite(PIN_LED_BLUE, blueNow);
			}
			else if (colorTransProg == 1) {
				colorTransProg = 0;
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
	const bool effectsOn = allowEffects && currentEffect != NONE;

	bool hasEffects = false;
	for (byte i = 0; i < 3; i++) {
		const uint16_t cur = displayData[i];
		if (dataToDisplayOld[i] != cur) {
			dataToDisplayPrevious[i] = dataToDisplayOld[i];
			dataToDisplayOld[i] = cur;
			if (effectsOn) {
				dataIsTransitioning[i] = EFFECT_SPEED;
			}
		}

		if (!effectsOn) {
			continue;
		}

		const byte tubeTrans = dataIsTransitioning[i];
		if (tubeTrans > 1) {
			if (currentEffect == SLOT_MACHINE) {
				byte effectData = getNumberBoth(tubeTrans / (EFFECT_SPEED / 10));
				const byte oldData = dataToDisplayPrevious[i];
				const byte nowData = dataToDisplayOld[i];
				if ((oldData & 0xF) == (nowData & 0xF)) {
					effectData &= ~0xF;
					effectData |= nowData & 0xF;
				}
				if ((oldData & 0xF0) == (nowData & 0xF0)) {
					effectData &= ~0xF0;
					effectData |= nowData & 0xF0;
				}
				displayData[i] = effectData;
			}
			dataIsTransitioning[i]--;
			hasEffects = true;
		}
		else if (tubeTrans == 1) {
			if (currentEffect == SLOT_MACHINE) {
				displayData[i] = dataToDisplayOld[i];
			}
			dataIsTransitioning[i] = 0;
		}
	}

	if (effectsOn && currentEffect == TRANSITION) {
		renderAlways = hasEffects;
	}

	lastCallMillis = curMillis;
}

void displayInit() {

}

void displayLoop() {
	const unsigned long curMicros = micros();
	if ((curMicros - lastDisplayRender) >= (DISPLAY_RENDER_STEP * 33UL)) {
		renderNixies();
		lastDisplayRender = curMicros;
	}
}
