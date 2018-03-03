#include "Display.h"
#include "DisplayTask.h"
#include "const.h"
#include <SPI.h>
#include <SoftTimer.h>

void renderNixies(Task *me);
Task T_renderNixies(5, renderNixies);

const uint16_t INVALID_TUBES = 10000;

#ifdef EFFECT_ENABLED
byte dataIsTransitioning[6] = { 0, 0, 0, 0, 0, 0 };
uint16_t dataToDisplayOld[6] = { INVALID_TUBES, INVALID_TUBES, INVALID_TUBES, INVALID_TUBES, INVALID_TUBES, INVALID_TUBES };
#endif
uint16_t dataToDisplay[6] = { 0, 0, 0, 0, 0, 0 }; // This will be displayed on tubes
byte dotMask;

const byte MASK_UPPER_DOTS = 1;
const byte MASK_LOWER_DOTS = 2;
const byte MASK_BOTH_DOTS = MASK_UPPER_DOTS | MASK_LOWER_DOTS;

unsigned long antiPoisonEnd = 0;

void displayInit() {
	SoftTimer.add(&T_renderNixies);
}

void displayAntiPoison(const unsigned long count) {
	antiPoisonEnd = millis() + (ANTI_POISON_DELAY * 10UL * count);
}

uint16_t getNumber(const byte idx) {
	return 1 << (idx % 10);
}

byte makeDotMask(const bool upper, const bool lower) {
	return (upper ? 0 : MASK_UPPER_DOTS) | (lower ? 0 : MASK_LOWER_DOTS);
}

void setDotsMask(const byte mask) {
	dotMask = mask;
}

void setDots(const bool upper, const bool lower) {
	setDotsMask(makeDotMask(upper, lower));
}

bool showShortTime(const unsigned long timeMs, bool trimLZ) {
	if (timeMs >= ONE_HOUR_IN_MS) { // Show H/M/S
		setDotsConst(true, false);
		trimLZ = insert2(0, (timeMs / ONE_HOUR_IN_MS) % 100, trimLZ);
		trimLZ = insert2(2, (timeMs / ONE_MINUTE_IN_MS) % 60, trimLZ);
		insert2(4, (timeMs / ONE_SECOND_IN_MS) % 60, trimLZ);
		return true;
	}
	else { // Show M/S/MS
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
	}
	else {
		dataToDisplay[offset] = getNumber(data);
	}
}

bool insert2(const byte offset, const byte data, const bool trimLeadingZero) {
	insert1(offset, data / 10, trimLeadingZero);
	insert1(offset + 1, data, trimLeadingZero);
	return data == 0;
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
#endif
}

void displayEffectsUpdate(const unsigned long microDelta) {
#ifdef EFFECT_ENABLED
	const unsigned long milliDelta = microDelta / 1000UL;
	bool hadEffects = false;
	for (byte i = 0; i < 6; i++) {
		if (dataIsTransitioning[i] > milliDelta) {
			dataIsTransitioning[i] -= milliDelta;
			hadEffects = true;
		}
		else {
			dataIsTransitioning[i] = 0;
		}
	}
#endif
}

void renderNixies(Task *me) {
	static byte anodeGroup = 0;

	const unsigned long curMillis = millis();
	uint16_t tubeL = INVALID_TUBES, tubeR = INVALID_TUBES;

	const unsigned long microDelta = me->nowMicros - me->lastCallTimeMicros;

	if (antiPoisonEnd > curMillis) {
		const uint16_t sym = getNumber((antiPoisonEnd - curMillis) / ANTI_POISON_DELAY);
		tubeL = sym;
		tubeR = sym;
		analogWrite(PIN_LED_RED, 0);
		analogWrite(PIN_LED_GREEN, 0);
		analogWrite(PIN_LED_BLUE, 0);
	}
	else if (DisplayTask::current) {
		if (DisplayTask::current->render(microDelta)) {
			displayTriggerEffects();
		}
		analogWrite(PIN_LED_RED, DisplayTask::current->red);
		analogWrite(PIN_LED_GREEN, DisplayTask::current->green);
		analogWrite(PIN_LED_BLUE, DisplayTask::current->blue);
	}

	displayEffectsUpdate(microDelta);

	const byte curTubeL = anodeGroup << 1;
	const byte curTubeR = curTubeL + 1;

	if (tubeL == INVALID_TUBES) {
		tubeL = dataToDisplay[curTubeL];
#ifdef EFFECT_ENABLED
		byte tubeTrans = dataIsTransitioning[curTubeL];
		if (tubeTrans > 0) {
#ifdef EFFECT_SLOT_MACHINE
			tubeL = getNumber(tubeTrans / (EFFECT_SPEED / 10));
#endif
		}
#endif
	}

	if (tubeR == INVALID_TUBES) {
		tubeR = dataToDisplay[curTubeR];
#ifdef EFFECT_ENABLED
		byte tubeTrans = dataIsTransitioning[curTubeR];
		if (tubeTrans > 0) {
#ifdef EFFECT_SLOT_MACHINE
			tubeR = getNumber(tubeTrans / (EFFECT_SPEED / 10));
#endif
		}
#endif
	}

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
