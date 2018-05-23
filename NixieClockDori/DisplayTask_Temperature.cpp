#include "DisplayTask_Temperature.h"
#include "Display.h"

#include <DS3232RTC.h>

bool DisplayTask_Temperature::_canShow() const {
	return false;
}

bool DisplayTask_Temperature::refresh() {
	DisplayTask::editMode = false;
	
	const unsigned long curMillis = millis();
	if ((curMillis - this->lastTempRefresh) >= 1000UL) {
		this->temp = RTC.temperature();
		this->lastTempRefresh = curMillis;
	}

	const int tempWhole = this->temp >> 2; // Divide by 4

	insert2(0, tempWhole / 100, true);
	insert2(2, tempWhole, true);
	switch (this->temp % 4) {
	case 0:
		insert2(4,  0, false);
		break;
	case 1:
		insert2(4, 25, false);
		break;
	case 2:
		insert2(4, 50, false);
		break;
	case 3:
		insert2(4, 75, false);
		break;
	}

	return true;
}
