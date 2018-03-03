#include "DisplayTask_Countdown.h"
#include "Display.h"

#include <TimeLib.h>

const bool DisplayTask_Countdown::_canShow() {
	return this->to != 0;
}

bool DisplayTask_Countdown::render(const unsigned long microDelta) {
	const unsigned long curMillis = millis();
	if (this->to < curMillis) {
		const uint16_t sym = (second() % 2) ? NO_TUBES : getNumber(0);
		for (byte i = 0; i < 6; i++) {
			this->dataToDisplay[i] = sym;
		}
		return false;
	}
	return showShortTime(this->to - curMillis, true, this->dataToDisplay, &this->dotMask);
}

