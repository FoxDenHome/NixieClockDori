#include "Display.h"
#include "DisplayTask.h"
#include "const.h"
#include <SPI.h>
#include <SoftTimer.h>

void renderNixies(Task *me);
Task T_renderNixies(5, renderNixies);

const uint16_t INVALID_TUBES = 10000;

const byte MASK_UPPER_DOTS = 1;
const byte MASK_LOWER_DOTS = 2;
const byte MASK_BOTH_DOTS = MASK_UPPER_DOTS | MASK_LOWER_DOTS;

unsigned long antiPoisonEnd = 0;

void displayInit() {
	SPI.begin();
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

void renderNixies(Task *me) {
#ifdef EFFECT_ENABLED
	static unsigned long dataIsTransitioning[6] = { 0, 0, 0, 0, 0, 0 };
	static uint16_t dataToDisplayOld[6] = { INVALID_TUBES, INVALID_TUBES, INVALID_TUBES, INVALID_TUBES, INVALID_TUBES, INVALID_TUBES };
#endif

	static byte anodeGroup = 0;
	const byte curTubeL = anodeGroup << 1;
	const byte curTubeR = curTubeL + 1;

	const unsigned long curMillis = millis();
	uint16_t tubeL = 0, tubeR = 0;
	byte dotMask = 0;

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
			// Start necessary effects (when display changed)
#ifdef EFFECT_ENABLED
			for (byte i = 0; i < 6; i++) {
				const uint16_t cur = DisplayTask::current->dataToDisplay[i];
				if (dataToDisplayOld[i] != cur) {
					dataToDisplayOld[i] = cur;
					dataIsTransitioning[i] = EFFECT_SPEED;
				}
			}
#endif
		}
		analogWrite(PIN_LED_RED, DisplayTask::current->red);
		analogWrite(PIN_LED_GREEN, DisplayTask::current->green);
		analogWrite(PIN_LED_BLUE, DisplayTask::current->blue);
		tubeL = DisplayTask::current->dataToDisplay[curTubeL];
		tubeR = DisplayTask::current->dataToDisplay[curTubeR];
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

	unsigned long tubeTrans = dataIsTransitioning[curTubeL];
	if (tubeTrans > 0) {
#ifdef EFFECT_SLOT_MACHINE
		tubeL = getNumber(tubeTrans / (EFFECT_SPEED / 10UL));
#endif
	}

	tubeTrans = dataIsTransitioning[curTubeR];
	if (tubeTrans > 0) {
#ifdef EFFECT_SLOT_MACHINE
		tubeR = getNumber(tubeTrans / (EFFECT_SPEED / 10UL));
#endif
	}
#endif

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
