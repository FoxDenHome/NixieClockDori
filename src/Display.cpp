#include "Display.h"
#include "DisplayTask.h"
#include "DisplayDriver.h"
#include "const.h"
#include "config.h"

unsigned long antiPoisonLeft = 0;
unsigned long lastDisplayRender = 0;

volatile uint16_t displayData[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

volatile unsigned long dataIsTransitioning[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
volatile bool renderAlways = false;
volatile bool renderNoMultiplex = false;

volatile byte dotMask = 0;
volatile bool allowEffects = false;

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

void renderSendToNixies(const bool isDirty) {
	static bool hasEffects = false;
	static uint16_t dataToDisplayTransitionEnd[9] = { NO_TUBES, NO_TUBES, NO_TUBES, NO_TUBES, NO_TUBES };
	static uint16_t dataToDisplayPrevious[9] = { NO_TUBES, NO_TUBES, NO_TUBES, NO_TUBES, NO_TUBES };
	static unsigned long lastCallMicros = 0;
	const unsigned long curMicros = micros();
	const unsigned long lastCallDelta = curMicros - lastCallMicros;

	const bool effectsOn = allowEffects && currentEffect != NONE;

	if (!isDirty && !hasEffects) {
		return;
	}

	bool needsRender = isDirty;
	hasEffects = false;

	for (byte i = 0; i < 9; i++) {
		const uint16_t cur = displayData[i];
		if (isDirty && dataToDisplayTransitionEnd[i] != cur) {
			dataToDisplayPrevious[i] = dataToDisplayTransitionEnd[i];
			dataToDisplayTransitionEnd[i] = cur;
			if (effectsOn) {
				dataIsTransitioning[i] = EFFECT_SPEED + lastCallDelta;
			}
		}

		if (!effectsOn) {
			continue;
		}

		uint16_t effectDo = cur;
		const uint16_t old = dataToDisplayPrevious[i];
		const unsigned long tubeTrans = dataIsTransitioning[i];
		if (tubeTrans > lastCallDelta) {
			const unsigned long transitionLeft = tubeTrans / (EFFECT_SPEED / 100UL);
			if (currentEffect == SLOT_MACHINE) {
				effectDo = getNumber(dataToDisplayTransitionEnd[i] + (transitionLeft / 10UL));
			} else if (currentEffect == TRANSITION) {
				effectDo = ((curMicros % 10UL) > (transitionLeft / 10UL)) ? cur : old;
			}
			dataIsTransitioning[i] -= lastCallDelta;
			hasEffects = true;
		}
		else if (tubeTrans > 0) {
			effectDo = dataToDisplayTransitionEnd[i];
			dataIsTransitioning[i] = 0;
		}

		if (effectDo != cur) {
			needsRender = true;
			displayData[i] = effectDo;
		}
	}

	if (needsRender) {
		displayDriverRefresh();
	}
	lastCallMicros = curMicros;
}

void renderNixies() {
	static byte oldAntiPoisonIdx = 255;
	static uint16_t antiPoisonTable[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	bool isDirty = false;

	static byte redOld = 1, greenOld = 1, blueOld = 1;

	static byte redPrevious = 1, greenPrevious = 1, bluePrevious = 1;
	static unsigned long colorTransProg = 0;

	const unsigned long curMicros = micros();
	static unsigned long lastCallMicros = 0;
	const unsigned long lastCallDelta = curMicros - lastCallMicros;

	if (antiPoisonLeft) {
		allowEffects = false;
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
			isDirty = true;
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

			if (colorTransProg > lastCallDelta && allowEffects) {
				colorTransProg -= lastCallDelta;
				const byte colorProgDivider = EFFECT_SPEED / colorTransProg;
				redNow = redOld + (redPrevious - redOld) / colorProgDivider;
				greenNow = greenOld + (greenPrevious - greenOld) / colorProgDivider;
				blueNow = blueOld + (bluePrevious - blueOld) / colorProgDivider;
				analogWrite(PIN_LED_RED, redNow);
				analogWrite(PIN_LED_GREEN, greenNow);
				analogWrite(PIN_LED_BLUE, blueNow);
			}
			else if (colorTransProg > 0) {
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

		isDirty = (dotMask != DisplayTask::current->dotMask) || DisplayTask::current->isDirty;
		if (isDirty) {
			dotMask = DisplayTask::current->dotMask;

			for (byte i = 0; i < 9; i++) {
				displayData[i] = DisplayTask::current->displayData[i];
			}

			DisplayTask::current->isDirty = false;
		}
	}

	renderSendToNixies(isDirty);

	lastCallMicros = curMicros;
}

void displayInit() {

}

void displayLoop() {
	const unsigned long curMicros = micros();
	if ((curMicros - lastDisplayRender) >= (DISPLAY_RENDER_STEP * 33UL)) {
		renderNixies();
		lastDisplayRender = curMicros;
		return;
	}
	renderSendToNixies(false);
}
