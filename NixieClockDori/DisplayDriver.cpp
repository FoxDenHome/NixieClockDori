#include "Display.h"
#include "DisplayDriver.h"
#include "const.h"
#include "config.h"
#include <SPI.h>

#define ALL_TUBES_MASK ((1 << 10) - 1)
#define NO_TUBES_MASK 0

// [456789__]
// [67890123]
// [89012345]
// [01234567]
#define SPI_3TUBE_XFER(A, B, C, DOTOFF) { \
	t1 = displayData[A]; \
	t2 = displayData[B]; \
	t3 = displayData[C]; \
	SPI.transfer(t3 >> 4 | ((lastSentDots >> DOTOFF) & 0x3) << 6); \
	SPI.transfer(t2 >> 6 | t3 << 4); \
	SPI.transfer(t1 >> 8 | t2 << 2); \
	SPI.transfer(t1); \
}

void displayDriverRefresh() {
	static uint16_t lastSentTubes[9] = { INVALID_TUBES, INVALID_TUBES, INVALID_TUBES, INVALID_TUBES, INVALID_TUBES, INVALID_TUBES, INVALID_TUBES, INVALID_TUBES, INVALID_TUBES };
	static byte lastSentDots = 0xFF;

	bool displayDiffers = lastSentDots != dotMask;
	for (int i = 0; i < 9; i++) {
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
	SPI_3TUBE_XFER(6, 7, 8, 4);
	SPI_3TUBE_XFER(3, 4, 5, 2);
	SPI_3TUBE_XFER(0, 1, 2, 0);
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
