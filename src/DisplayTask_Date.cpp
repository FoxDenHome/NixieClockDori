#include "DisplayTask_Date.h"
#include "Display.h"
#include "config.h"
#include "const.h"
#include "rtc.h"

#include <EEPROM.h>
#include <TimeLib.h>

DisplayTask_Date::DisplayTask_Date() {

}

void DisplayTask_Date::loadConfig(const int16_t base) {
	this->base = base;
	if (base < 0) {
		return;
	}

	EEPROM.get(base, this->cycleAuto);
}

bool DisplayTask_Date::_isActive() const {
	return this->cycleAuto;
}

void DisplayTask_Date::handleButtonPress(const Button button, const PressType pressType) {
	if ((button == UP || button == DOWN) && pressType == Click && !this->editMode) {
		this->cycleAuto = !this->cycleAuto;
		if (this->base >= 0) {
			EEPROM.put(this->base, this->cycleAuto);
		}
		return;
	}
	DisplayTask::handleButtonPress(button, pressType);
}

void DisplayTask_Date::handleEdit(const byte digit, const bool up) {
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
	tm.Year = y2kYearToTm(y);
	tm.Month = m;
	tm.Day = d;
	rtcSetTime(tm);
}

bool DisplayTask_Date::refresh() {
	bool dateChanged = false;

	if (this->cycleAuto) {
		this->dotMask = DOT_1_DOWN | DOT_2_DOWN;
	}
	else {
		this->dotMask = DOT_1_UP | DOT_2_UP;
	}

	if (!DisplayTask::editMode) {
		const time_t _n = now();
		const byte y = year(_n) % 100;
		const byte m = month(_n);
		const byte d = day(_n);

		dateChanged = (this->d != d || this->m != m || this->y != y);

		this->y = y;
		this->m = m;
		this->d = d;
	}
	else {
		this->setDisplayData(6, NO_TUBES);
		this->setDisplayData(7, NO_TUBES);
		this->setDisplayData(8, NO_TUBES);
		dateChanged = true;
	}

	if (!DisplayTask::editMode) {
		dateChanged |= this->insertTemp();
	}

	if (!dateChanged) {
		return true;
	}

	insert2(0, this->d, false);
	insert2(2, this->m, false);
	insert2(4, this->y, false);

	return DisplayTask::refresh();
}

