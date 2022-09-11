#include "Display.h"
#include "DisplayDriver.h"
#include "const.h"
#include "config.h"
#include <SPI.h>

//   Number      Tube
// [456789DU] [333333DD]
// [67890123] [22223333]
// [89012345] [11222222]
// [01234567] [11111111]
#define SPI_3TUBE_XFER(A, B, C, DOTOFF) { \
	t1 = displayData[A]; \
	t2 = displayData[B]; \
	t3 = displayData[C]; \
	SPI.transfer(t3 >> 4 | ((dotMask >> DOTOFF) & 0x3) << 6); \
	SPI.transfer(t2 >> 6 | t3 << 4); \
	SPI.transfer(t1 >> 8 | t2 << 2); \
	SPI.transfer(t1); \
}

void displayDriverBlank() {
	SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE2));
	digitalWrite(PIN_DISPLAY_LATCH, LOW);
	SPI.transfer(0); SPI.transfer(0); SPI.transfer(0); SPI.transfer(0);
	SPI.transfer(0); SPI.transfer(0); SPI.transfer(0); SPI.transfer(0);
	SPI.transfer(0); SPI.transfer(0); SPI.transfer(0); SPI.transfer(0);
	digitalWrite(PIN_DISPLAY_LATCH, HIGH);
	SPI.endTransaction();
}

void displayDriverRefresh() {
	uint16_t t1, t2, t3;

	SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE2));
	digitalWrite(PIN_DISPLAY_LATCH, LOW);
	SPI_3TUBE_XFER(6, 7, 8, 4);
	SPI_3TUBE_XFER(3, 4, 5, 2);
	SPI_3TUBE_XFER(0, 1, 2, 0);
	digitalWrite(PIN_DISPLAY_LATCH, HIGH);
	SPI.endTransaction();
}

void displayDriverInit() {
	SPI.begin();
}

void displayDriverLoop() {

}
