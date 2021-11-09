#include "DisplayTask_Flash.h"
#include "Display.h"
#include "crcserial.h"

static uint16_t charToTube(const byte chr) {
	switch (chr) {
		// Special tube layouts
	case ' ':
		return NO_TUBES;
		break;
	case '*':
		return ALL_TUBES;
		break;

		// Symbols for IN-19
	case '%':
		return SYMBOL_PERCENT;
		break;
	case 'M':
		return SYMBOL_M;
		break;
	case 'P':
		return SYMBOL_P;
		break;
	case 'm':
		return SYMBOL_m;
		break;
	case 'K':
		return SYMBOL_K;
		break;
	case 'n':
		return SYMBOL_n;
		break;
	case 'u':
		return SYMBOL_MICRO;
		break;
	case 'C':
		return SYMBOL_DEGREES_C;
		break;

	default:
		if (chr < '0' || chr > '9') {
			return NO_TUBES;
		}

		return getNumber(chr - '0');
		break;
	}
}

DisplayTask_Flash::DisplayTask_Flash() {
	this->removeOnCantShow = true;
}

bool DisplayTask_Flash::_canShow() const {
	return this->endTime > 0 && millis() < this->endTime;
}

void DisplayTask_Flash::setDataFromSerial() {
	const unsigned long curMillis = millis();

	this->allowEffects = (curMillis - this->lastUpdate) >= 400;
	this->lastUpdate = curMillis;
	this->endTime = curMillis + (unsigned long)inputString.substring(1, 9).toInt();

	this->dotMask = (inputString[9] - '0') | ((inputString[10] - '0') << 2) | ((inputString[11] - '0') << 4);

	for (byte i = 0; i < 9; i++) {
		this->setDisplayData(i, charToTube(inputString[i + 12]));
	}

	this->setColorFromInput(16, -1);
	this->showIfPossibleOtherwiseRotateIfCurrent();
}

bool DisplayTask_Flash::refresh() {
	DisplayTask::editMode = false;
	if (!this->canShow()) {
		cycleDisplayUpdater();
	}

	return this->allowEffects;
}

