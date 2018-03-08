#include "Display.h"
#include "DisplayDriver.h"
#include "const.h"
#include "config.h"
#include <SPI.h>
#include <TimerOne.h>

uint16_t lastSentTubes[6] = { 9999, 9999, 9999, 9999, 9999, 9999 };

void displayInterrupt() {
	static byte ctr = 0;

	const byte ctrL = ctr % 11;
	if (ctrL <= 1 || renderAlways) {
		const byte anodeGroup = ctr / 11;
		const byte anodeControl = ctrL ? (1 << (anodeGroup + 4)) : 0;

		const byte curTubeL = anodeGroup << 1;
		const byte curTubeR = curTubeL + 1;

		uint16_t tubeL = displayDataFront[curTubeL];
		uint16_t tubeR = displayDataFront[curTubeR];
		if (ctrL && renderAlways && currentEffect == TRANSITION) {
			if (ctrL <= (dataIsTransitioning[curTubeL] / (EFFECT_SPEED / 10UL))) {
				tubeL = dataToDisplayPrevious[curTubeL];
			}
			if (ctrL <= (dataIsTransitioning[curTubeR] / (EFFECT_SPEED / 10UL))) {
				tubeR = dataToDisplayPrevious[curTubeR];
			}
		}


		// We don't need to un-blank if an entire segment is blank
		if (lastSentTubes[curTubeL] != tubeL || lastSentTubes[curTubeR] != tubeR || ctrL <= 1) {
			if (anodeControl) {
				lastSentTubes[curTubeL] = tubeL;
				lastSentTubes[curTubeR] = tubeR;
			}

			PORT_DISPLAY_LATCH &= ~PORT_MASK_DISPLAY_LATCH;
			SPI.transfer(dotMask);                            // [   ][   ][   ][   ][   ][   ][L1 ][L0 ] - L0     L1 - dots
			SPI.transfer(tubeR >> 6 | anodeControl);          // [   ][A2 ][A1 ][A0 ][RC9][RC8][RC7][RC6] - A0  -  A2 - anodes
			SPI.transfer(tubeR << 2 | tubeL >> 8);            // [RC5][RC4][RC3][RC2][RC1][RC0][LC9][LC8] - RC9 - RC0 - Right tubes cathodes
			SPI.transfer(tubeL);                              // [LC7][LC6][LC5][LC4][LC3][LC2][LC1][LC0] - LC9 - LC0 - Left tubes cathodes
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
