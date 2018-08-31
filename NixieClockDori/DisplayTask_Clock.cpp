#include "DisplayTask_Clock.h"
#include "Display.h"
#include "config.h"
#include "const.h"
#include "rtc.h"

#include <EEPROM.h>
#include <TimeLib.h>

void DisplayTask_Clock::handleButtonPress(const Button button, const PressType pressType) {
	if ((button == UP || button == DOWN) && pressType == Click && !this->editMode) {
		currentEffect = static_cast<DisplayEffect>(static_cast<byte>(currentEffect) + 1);
		if (currentEffect == FIRST_INVALID) {
			currentEffect = NONE;
		}
		const DisplayEffect saveEffect = currentEffect;
		EEPROM.put(EEPROM_STORAGE_CURRENT_EFFECT, saveEffect);
		return;
	}
	DisplayTask::handleButtonPress(button, pressType);
}

void DisplayTask_Clock::handleEdit(const byte digit, const bool up) {
	this->_handleEditHelper(digit, up, h, m, s, 23, 59, 59);

	tmElements_t tm;
	breakTime(now(), tm);
	tm.Hour = h;
	tm.Minute = m;
	tm.Second = s;
	rtcSetTime(tm);
}

bool DisplayTask_Clock::refresh() {
	if (!DisplayTask::editMode) {
		const time_t _n = now();
		const byte h = hour(_n);
		const byte m = minute(_n);
		const byte s = second(_n);

		if (this->s != s) {
			if (h < 4) {
				if (s % 5 == 2) {
					displayAntiPoison(1);
				}
			}
			else if (s == 2 && m % 10 == 2) {
				displayAntiPoison(2);
			}
		}

		this->h = h;
		this->m = m;
		this->s = s;
	}

	if (this->s % 2 || DisplayTask::editMode) {
		this->dotMask = makeDotMask(true, true);
	}
	else {
		this->dotMask = makeDotMask(false, false);
	}

#ifdef CLOCK_TRIM_HOURS
	insert1(0, this->h / 10, true);
	insert1(1, this->h, false);
#else
	insert2(0, this->h, false);
#endif
	insert2(2, this->m, false);
	insert2(4, this->s, false);

	return DisplayTask::refresh();
}

