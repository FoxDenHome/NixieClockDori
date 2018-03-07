#include "Display.h"
#include "DisplayTask.h"
#include "const.h"
#include "config.h"

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

	bool allowEffects = false, doFlip = true;

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
				//analogWrite(PIN_LED_RED, redNow);
				Timer1.pwm(PIN_LED_RED, redNow << 2);
				analogWrite(PIN_LED_GREEN, greenNow);
				analogWrite(PIN_LED_BLUE, blueNow);
			}
			else if (colorTransProg >= 0) {
				colorTransProg = -1;
				//analogWrite(PIN_LED_RED, redNow);
				Timer1.pwm(PIN_LED_RED, redNow << 2);
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
				//analogWrite(PIN_LED_RED, redOld);
				Timer1.pwm(PIN_LED_RED, redOld << 2);
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
	if (allowEffects && currentEffect != NONE) {
		bool hasEffects = false;
		for (byte i = 0; i < 6; i++) {
			const uint16_t cur = displayDataBack[i];
			if (dataToDisplayOld[i] != cur) {
				dataToDisplayPrevious[i] = dataToDisplayOld[i];
				dataToDisplayOld[i] = cur;
				dataIsTransitioning[i] = EFFECT_SPEED;
			}

			const unsigned long tubeTrans = dataIsTransitioning[i];

			if (tubeTrans > microDelta) {
				if (currentEffect == SLOT_MACHINE) {
					displayDataBack[i] = getNumber(tubeTrans / (EFFECT_SPEED / 10UL));
				}
				dataIsTransitioning[i] -= microDelta;
				hasEffects = true;
			}
			else if (tubeTrans > 0) {
				displayDataBack[i] = dataToDisplayOld[i];
				dataIsTransitioning[i] = 0;
			}
		}
		if (currentEffect == TRANSITION) {
			renderAlways = hasEffects;
		}
		doFlip = true;
	}

	if (doFlip) {
		uint16_t *tmp = displayDataFront;
		displayDataFront = displayDataBack;
		displayDataBack = tmp;
	}
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
