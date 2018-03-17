#include "DisplayTask_Flash.h"
#include "Display.h"

DisplayTask_Flash::DisplayTask_Flash() {
	this->removeOnCantShow = true;
}

bool DisplayTask_Flash::_canShow() const {
	return this->endTime > 0 && millis() < this->endTime;
}

bool DisplayTask_Flash::refresh() {
	DisplayTask::editMode = false;
	if (!this->canShow()) {
		cycleDisplayUpdater();
	}

	for (byte i = 0; i < 3; i++) {
		displayData[i] = this->symbols[i];
	}
	return this->allowEffects;
}

