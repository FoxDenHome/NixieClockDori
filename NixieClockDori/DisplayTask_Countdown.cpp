#include "DisplayTask_Countdown.h"
#include "Display.h"

#include <TimeLib.h>

const bool DisplayTask_Countdown::_canShow() {
	return this->to != 0;
}

bool DisplayTask_Countdown::refresh(uint16_t displayData[]) {
	const unsigned long curMillis = millis();

	if (this->to < curMillis) {
		const uint16_t osym = (second() % 2) ? NO_TUBES : getNumber(0);
		for (byte i = 0; i < 6; i++) {
			displayData[i] = osym;
		}
		return false;
	}

	return showShortTime(this->to - curMillis, true, displayData, &this->dotMask);
}
