#include "Display.h"
#include "DisplayDriver.h"
#include "const.h"
#include "config.h"
#include <SPI.h>
#include <TimerOne.h>

#define ALL_TUBES_MASK ((1 << 10) - 1)
#define NO_TUBES_MASK 0

#define GET_ANODE_MASK(a) (1 << (a + 4))
#define ALL_ANODES_MASK (GET_ANODE_MASK(0) | GET_ANODE_MASK(1) | GET_ANODE_MASK(2))

#pragma GCC push_options
#pragma GCC optimize ("O3")
uint16_t inline mkTube(const byte idx) {
	switch (idx) {
	case ALL_TUBES:
		return ALL_TUBES_MASK;
	case NO_TUBES:
		return NO_TUBES_MASK;
	default:
		if (idx > 9) {
			return 0;
		}
		return 1 << idx;
	}
}

void displayInterrupt() {
	static byte lastSentTubes[3] = { INVALID_TUBES_BOTH, INVALID_TUBES_BOTH, INVALID_TUBES_BOTH };
	static byte ctr = 0;

	const byte ctrL = ctr % 11;
	if (ctrL <= 1 || renderAlways) {
		const byte anodeGroup = renderNoMultiplex ? 0 : (ctr / 11);
		const byte anodeMask = renderNoMultiplex ? ALL_ANODES_MASK : GET_ANODE_MASK(anodeGroup);

		byte tubei = displayData[anodeGroup];
		if (ctrL && renderAlways && currentEffect == TRANSITION && ctrL <= (dataIsTransitioning[anodeGroup] / (EFFECT_SPEED / 10))) {
			tubei = dataToDisplayPrevious[anodeGroup];
		}

		// We don't need to refresh if it is just the same as last time (unless we are in main phase render)
		if (lastSentTubes[anodeGroup] != tubei || (ctrL <= 1 && !renderNoMultiplex)) {
			const uint16_t tubeL = mkTube(tubei & 0xF);
			const uint16_t tubeR = mkTube(tubei >> 4);

			PORT_DISPLAY_LATCH &= ~PORT_MASK_DISPLAY_LATCH;
			SPI.transfer(dotMask);                                  // [   ][   ][   ][   ][   ][   ][L1 ][L0 ] - L0     L1 - dots
			if (ctrL || renderNoMultiplex) {
				lastSentTubes[anodeGroup] = tubei;
				SPI.transfer(tubeR >> 6 | anodeMask);               // [   ][A2 ][A1 ][A0 ][RC9][RC8][RC7][RC6] - A0  -  A2 - anodes (displaying)
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
#pragma GCC pop_options

void displayDriverInit() {
	SPI.begin();
	SPI.setDataMode(SPI_MODE2);
	SPI.setClockDivider(SPI_CLOCK_DIV4);
	SPI.setBitOrder(MSBFIRST);
	SPI.usingInterrupt(255);
	Timer1.initialize(DISPLAY_RENDER_STEP);
	Timer1.attachInterrupt(&displayInterrupt);
}

void displayDriverLoop() {

}
