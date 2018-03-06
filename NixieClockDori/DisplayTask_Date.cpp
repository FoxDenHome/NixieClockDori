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
	switch (digit) {
	case 0:
		if (up) {
			if (d >= 30 || (d >= 20 && (d % 10) > 1)) {
				d %= 10;
			}
			else {
				d += 10;
			}
		}
		else {
			if (d < 10) {
				d = (((d % 10) > 1) ? 20 : 30) + (d % 10);
			}
			else {
				d -= 10;
			}
		}
		break;
	case 1:
		if (up) {
			if ((d % 10) == 9 || ((d % 10) > 1 && d >= 30)) {
				d -= d % 10;
			}
			else {
				d += 1;
			}
		}
		else {
			if ((d % 10) == 0) {
				d -= (d % 10) - ((d >= 30) ? 1 : 9);
			}
			else {
				d -= 1;
			}
		}
		break;
	case 2:
		if (up) {
			if (m > 2) {
				m %= 10;
			}
			else {
				m += 10;
			}
		}
		else {
			if (m <= 2) {
				m = 10 + (m % 10);
			}
			else if (m >= 10) {
				m -= 10;
			}
		}
		break;
	case 3:
		if (up) {
			if ((m % 10) == 9 || (m >= 12)) {
				m -= m % 10;
			}
			else {
				m += 1;
			}
		}
		else {
			if (m == 10) {
				m = 12;
			}
			else if (m == 0) {
				m = 9;
			}
			else {
				m -= 1;
			}
		}
		break;
	case 4:
		if (up) {
			if (y >= 90) {
				y %= 10;
			}
			else {
				y += 10;
			}
		}
		else {
			if (y < 10) {
				y = 90 + y % 10;
			}
			else {
				y -= 10;
			}
		}
		break;
	case 5:
		if (up) {
			if ((y % 10) == 9) {
				y -= 9;
			}
			else {
				y += 1;
			}
		}
		else {
			if ((y % 10) == 0) {
				y += 9;
			}
			else {
				y -= 1;
			}
		}
		break;
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

