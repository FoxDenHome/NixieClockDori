#include "Display.h"
#include "DisplayDriver.h"
#include "const.h"
#include "config.h"
#include <SPI.h>
#include <TimerOne.h>

byte lastSentTubes[6] = { INVALID_TUBES, INVALID_TUBES, INVALID_TUBES, INVALID_TUBES, INVALID_TUBES, INVALID_TUBES };

#define ALL_TUBES_MASK ((1 << 10) - 1)
#define NO_TUBES_MASK 0

uint16_t inline mkTube(const byte idx) {
	switch (idx) {
	case ALL_TUBES:
		return ALL_TUBES_MASK;
	case NO_TUBES:
		return NO_TUBES_MASK;
	default:
		return 1 << idx;
	}
}

void displayInterrupt() {
	static byte ctr = 0;

	const byte ctrL = ctr % 11;
	if (ctrL <= 1 || renderAlways) {
		const byte anodeGroup = ctr / 11;

		const byte curTubeL = anodeGroup << 1;
		const byte curTubeR = curTubeL + 1;

		byte tubeLi = displayData[curTubeL];
		byte tubeRi = displayData[curTubeR];
		if (ctrL && renderAlways && currentEffect == TRANSITION) {
			if (ctrL <= (dataIsTransitioning[curTubeL] / (EFFECT_SPEED / 10))) {
				tubeLi = dataToDisplayPrevious[curTubeL];
			}
			if (ctrL <= (dataIsTransitioning[curTubeR] / (EFFECT_SPEED / 10))) {
				tubeRi = dataToDisplayPrevious[curTubeR];
			}
		}

		// We don't need to un-blank if an entire segment is blank
		if (lastSentTubes[curTubeL] != tubeLi || lastSentTubes[curTubeR] != tubeRi || ctrL <= 1) {
			const uint16_t tubeL = mkTube(tubeLi);
			const uint16_t tubeR = mkTube(tubeRi);

			PORT_DISPLAY_LATCH &= ~PORT_MASK_DISPLAY_LATCH;
			SPI.transfer(dotMask);                                  // [   ][   ][   ][   ][   ][   ][L1 ][L0 ] - L0     L1 - dots
			if (ctrL) {
				lastSentTubes[curTubeL] = tubeLi;
				lastSentTubes[curTubeR] = tubeRi;
				SPI.transfer(tubeR >> 6 | (1 << (anodeGroup + 4))); // [   ][A2 ][A1 ][A0 ][RC9][RC8][RC7][RC6] - A0  -  A2 - anodes (displaying)
			}
			else {
				SPI.transfer(tubeR >> 6);                           // [   ][A2 ][A1 ][A0 ][RC9][RC8][RC7][RC6] - A0  -  A2 - anodes (blanking)
			}
			SPI.transfer(tubeR << 2 | tubeL >> 8);                  // [RC5][RC4][RC3][RC2][RC1][RC0][LC9][LC8] - RC9 - RC0 - Right tubes cathodes
			SPI.transfer(tubeL);                                    // [LC7][LC6][LC5][LC4][LC3][LC2][LC1][LC0] - LC9 - LC0 - Left tubes cathodes
			PORT_DISPLAY_LATCH |= PORT_MASK_DISPLAY_LATCH;
		}
	}

	if (++ctr > 33) {
		ctr = 0;
	}
}

void displayDriverInit() {
	SPI.begin();
	SPI.setDataMode(SPI_MODE2);
	SPI.setClockDivider(SPI_CLOCK_DIV4);
	SPI.setBitOrder(MSBFIRST);
	SPI.usingInterrupt(255);
	Timer1.initialize(DISPLAY_RENDER_STEP);
	Timer1.attachInterrupt(&displayInterrupt);
}

void displayDriverLoop(const unsigned long curMicros) {

}
