#include "DisplayTask_Flash.h"
#include "Display.h"

const bool DisplayTask_Flash::_canShow() {
	return this->endTime > 0 && millis() < this->endTime;
}

bool DisplayTask_Flash::render() {
	return true;
}

