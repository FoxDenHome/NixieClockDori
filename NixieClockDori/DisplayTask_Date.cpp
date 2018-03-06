#include "DisplayTask_Date.h"
#include "Display.h"
#include "config.h"
#include "const.h"
#include "rtc.h"

#include <EEPROM.h>
#include <TimeLib.h>

DisplayTask_Date::DisplayTask_Date() {
	EEPROM.get(EEPROM_STORAGE_DATE_AUTO, this->cycleAuto);
	this->dotMask = makeDotMask(!this->cycleAuto, this->cycleAuto);
}

const bool DisplayTask_Date::_canShow() {
	return this->cycleAuto;
}

void DisplayTask_Date::handleButtonPress(Button button, PressType pressType) {
	if ((button == UP || button == DOWN) && pressType == Click && !this->editMode) {
		this->cycleAuto = !this->cycleAuto;
		EEPROM.put(EEPROM_STORAGE_DATE_AUTO, this->cycleAuto);
		this->dotMask = makeDotMask(!this->cycleAuto, this->cycleAuto);
		return;
	}
	DisplayTask::handleButtonPress(button, pressType);
}

void DisplayTask_Date::handleEdit(byte digit, bool up) {
	this->_handleEditHelper(digit, up, d, m, y, 31, 12, 99);

	if (digit == 255) {
		if (d == 0) {
			d = 1;
		}
		if (m == 0) {
			m = 1;
		}
	}

	tmElements_t tm;
	breakTime(now(), tm);
	tm.Year = y + 30; // Offset from 1970
	tm.Month = m;
	tm.Day = d;
	rtcSetTime(tm);
}

bool DisplayTask_Date::refresh(uint16_t displayData[]) {
	if (!DisplayTask::editMode) {
		const time_t _n = now();
		const byte y = year(_n) % 100;
		const byte m = month(_n);
		const byte d = day(_n);

		this->y = y;
		this->m = m;
		this->d = d;
	}

	insert2(0, this->d, false, displayData);
	insert2(2, this->m, false, displayData);
	insert2(4, this->y, false, displayData);

	return DisplayTask::refresh(displayData);
}

