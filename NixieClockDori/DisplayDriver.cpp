#include "Display.h"
#include "DisplayDriver.h"
#include "const.h"
#include "config.h"
#include <SPI.h>

#define ALL_TUBES_MASK ((1 << 10) - 1)
#define NO_TUBES_MASK 0

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

#define GET_TUBE_L(x) (displayData[x] & 0xF)
#define GET_TUBE_R(x) (displayData[x] >> 4)


// [456789__]
// [67890123]
// [89012345]
// [01234567]
#define SPI_3TUBE_XFER(A, B, C, DOTOFF) { \
	t1 = mkTube(A); \
	t2 = mkTube(B); \
	t3 = mkTube(C); \
	SPI.transfer(t3 >> 4 | ((dotMask >> DOTOFF) & 0x3) << 6); \
	SPI.transfer(t2 >> 6 | t3 << 4); \
	SPI.transfer(t1 >> 8 | t2 << 2); \
	SPI.transfer(t1); \
}

void displayDriverRefresh() {
	static byte lastSentTubes[5] = { INVALID_TUBES_BOTH, INVALID_TUBES_BOTH, INVALID_TUBES_BOTH, INVALID_TUBES_BOTH, INVALID_TUBES_BOTH };
	static byte lastSentDots = 0xFF;

	bool displayDiffers = lastSentDots != dotMask;
	for (int i = 0; i < 5; i++) {
		if (lastSentTubes[i] != displayData[i]) {
			lastSentTubes[i] = displayData[i];
			displayDiffers = true;
		}
	}

	if (!displayDiffers) {
		return;
	}

	lastSentDots = dotMask;

	uint16_t t1, t2, t3;
	digitalWrite(PIN_DISPLAY_LATCH, LOW);
	SPI_3TUBE_XFER(GET_TUBE_L(3), GET_TUBE_R(3), GET_TUBE_L(4), 4);
	SPI_3TUBE_XFER(GET_TUBE_R(1), GET_TUBE_L(2), GET_TUBE_R(2), 2);
	SPI_3TUBE_XFER(GET_TUBE_L(0), GET_TUBE_R(0), GET_TUBE_L(1), 0);
	digitalWrite(PIN_DISPLAY_LATCH, HIGH);

}

void displayDriverInit() {
	SPI.begin();
	SPI.setDataMode(SPI_MODE2);
	SPI.setClockDivider(SPI_CLOCK_DIV8);
	SPI.setBitOrder(MSBFIRST);
}

void displayDriverLoop() {

}
