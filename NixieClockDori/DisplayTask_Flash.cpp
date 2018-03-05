#include "DisplayTask_Flash.h"
#include "Display.h"

DisplayTask_Flash::DisplayTask_Flash() {
	this->removeOnCantShow = true;
}

const bool DisplayTask_Flash::_canShow() {
	return this->endTime > 0 && millis() < this->endTime;
}

bool DisplayTask_Flash::refresh(uint16_t displayData[]) {
	DisplayTask::editMode = false;
	if (!this->canShow()) {
		cycleDisplayUpdater();
	}

	for (byte i = 0; i < 6; i++) {
		displayData[i] = this->symbols[i];
	}
	return true;
}

