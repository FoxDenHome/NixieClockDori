#include "Display.h"
#include "DisplayTask.h"
#include "DisplayDriver.h"
#include "const.h"
#include "config.h"

unsigned long antiPoisonLeft = 0;
unsigned long lastDisplayRender = 0;

volatile uint16_t displayData[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

volatile byte dataIsTransitioning[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
volatile uint16_t dataToDisplayPrevious[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
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
	trimLZ = insert2(0, (timeMs / ONE_HOUR_IN_MS) % 100, trimLZ);
	trimLZ = insert2(2, (timeMs / ONE_MINUTE_IN_MS) % 60, trimLZ);
	trimLZ = insert2(4, (timeMs / ONE_SECOND_IN_MS) % 60, trimLZ);
	insert2(6, (timeMs / 10UL) % 100, trimLZ);
	return false;
}

void insert1(const byte offset, const byte data, const bool trimLeadingZero) {
	if (data == 0 && trimLeadingZero) {
		displayData[offset] = NO_TUBES;
	}
	else {
		displayData[offset] = getNumber(data);
	}
}

bool insert2(const byte offset, const byte data, const bool trimLeadingZero) {
	insert1(offset, data / 10, trimLeadingZero);
	insert1(offset + 1, data, trimLeadingZero);
	return data == 0 && trimLeadingZero;
}

void renderNixies() {
	static byte oldAntiPoisonIdx = 255;
	static uint16_t antiPoisonTable[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	bool allowEffects = false;

	static byte redOld = 1, greenOld = 1, blueOld = 1;

	static uint16_t dataToDisplayOld[9] = { NO_TUBES, NO_TUBES, NO_TUBES, NO_TUBES, NO_TUBES };
	static byte redPrevious, greenPrevious, bluePrevious;
	static byte colorTransProg;

	const unsigned long curMillis = millis();
	static unsigned long lastCallMillis = 0;
	const unsigned long lastCallDelta = curMillis - lastCallMillis;

	if (antiPoisonLeft) {
		const byte idx = 9 - ((antiPoisonLeft / ANTI_POISON_DELAY) % 10);
		if (idx != oldAntiPoisonIdx) {
			for (byte i = 0; i < 9; i++) {
				if (antiPoisonTable[i] == ALL_TUBES) {
					antiPoisonTable[i] = 0;
				}
				byte randNbr = random(0, 10);
				while (antiPoisonTable[i] & (1 << randNbr)) {
					if (++randNbr > 9) {
						randNbr = 0;
					}
				}
				const uint16_t nbrTube = (1 << randNbr);
				antiPoisonTable[i] |= nbrTube;
				displayData[i] = nbrTube;
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

		dotMask = DOT_1_DOWN | DOT_1_UP | DOT_2_DOWN | DOT_2_UP | DOT_3_DOWN | DOT_3_UP;
	}
	else {
		allowEffects = DisplayTask::current->refresh();

		if (currentEffect != NONE) {
			byte redNow = redOld, greenNow = greenOld, blueNow = blueOld;

			if (colorTransProg > 1 && allowEffects) {
				colorTransProg--;
				redNow = redOld + (((redPrevious - redOld) * colorTransProg) / EFFECT_SPEED);
				greenNow = greenOld + (((greenPrevious - greenOld) * colorTransProg) / EFFECT_SPEED);
				blueNow = blueOld + (((bluePrevious - blueOld) * colorTransProg) / EFFECT_SPEED);
				analogWrite(PIN_LED_RED, redNow);
				analogWrite(PIN_LED_GREEN, greenNow);
				analogWrite(PIN_LED_BLUE, blueNow);
			}
			else if (colorTransProg == 1) {
				colorTransProg = 0;
				analogWrite(PIN_LED_RED, redNow);
				analogWrite(PIN_LED_GREEN, greenNow);
				analogWrite(PIN_LED_BLUE, blueNow);
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
		}

		dotMask = DisplayTask::current->dotMask;
	}

	// Progress through effect
	const bool effectsOn = allowEffects && currentEffect != NONE;

	//bool hasEffects = false;
	for (byte i = 0; i < 9; i++) {
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
				displayData[i] = getNumber(tubeTrans / (EFFECT_SPEED / 10));
			}
			dataIsTransitioning[i]--;
			//hasEffects = true;
		}
		else if (tubeTrans == 1) {
			if (currentEffect == SLOT_MACHINE) {
				displayData[i] = dataToDisplayOld[i];
			}
			dataIsTransitioning[i] = 0;
		}
	}

	//if (effectsOn && currentEffect == TRANSITION) {
	//	renderAlways = hasEffects;
	//}

	displayDriverRefresh();

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
