#include "DisplayTask_Flash.h"
#include "Display.h"

const bool DisplayTask_Flash::_canShow() {
	return this->endTime > 0 && millis() < this->endTime;
}

bool DisplayTask_Flash::refresh(uint16_t displayData[]) {
	for (byte i = 0; i < 6; i++) {
		displayData[i] = this->symbols[i];
	}
	return true;
}

