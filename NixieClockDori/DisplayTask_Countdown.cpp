#include "DisplayTask_Countdown.h"
#include "Display.h"
#include "const.h"

#include <EEPROM.h>
#include <TimeLib.h>

#define COUNTDOWN_MAX_TIME (100UL * 60UL * 60UL * 1000UL)

DisplayTask_Countdown::DisplayTask_Countdown() {
	this->dotMask = makeDotMask(false, true);
	EEPROM.get(EEPROM_STORAGE_COUNTDOWN, this->timeReset);
	if (!this->timeReset || this->timeReset >= COUNTDOWN_MAX_TIME) {
		this->timeReset = 10000;
	}
	this->reset();
}

void DisplayTask_Countdown::handleEdit(byte digit, bool up) {
	unsigned long tDiff = 0UL;
	byte h = this->timeReset / (60UL * 60UL * 1000UL);
	byte m = (this->timeReset / (60UL * 1000UL)) % 60;
	byte s = (this->timeReset / 1000UL) % 60;

	switch (digit) {
	case 0:
		if (up) {
			if (h >= 90) {
				h %= 10;
			}
			else {
				h += 10;
			}
		}
		else {
			if (h < 10) {
				h = 90 + h % 10;
			}
			else {
				h -= 10;
			}
		}
		break;
	case 1:
		if (up) {
			if ((h % 10) == 9) {
				h -= 9;
			}
			else {
				h += 1;
			}
		}
		else {
			if ((h % 10) == 0) {
				h += 9;
			}
			else {
				h -= 1;
			}
		}
		break;
	case 2:
		if (up) {
			if (m >= 50) {
				m %= 10;
			}
			else {
				m += 10;
			}
		}
		else {
			if (m < 10) {
				m = 50 + m % 10;
			}
			else {
				m -= 10;
			}
		}
		break;
	case 3:
		if (up) {
			if ((m % 10) == 9) {
				m -= 9;
			}
			else {
				m += 1;
			}
		}
		else {
			if ((m % 10) == 0) {
				m += 9;
			}
			else {
				m -= 1;
			}
		}
		break;
	case 4:
		if (up) {
			if (s >= 50) {
				s %= 10;
			}
			else {
				s += 10;
			}
		}
		else {
			if (s < 10) {
				s = 50 + s % 10;
			}
			else {
				s -= 10;
			}
		}
		break;
	case 5:
		if (up) {
			if ((s % 10) == 9) {
				s -= 9;
			}
			else {
				s += 1;
			}
		}
		else {
			if ((s % 10) == 0) {
				s += 9;
			}
			else {
				s -= 1;
			}
		}
		break;
	}
	
	this->timeReset = ((((h * 60UL) + m) * 60UL) + s) * 1000UL;

	if (this->timeReset >= COUNTDOWN_MAX_TIME) {
		this->timeReset %= COUNTDOWN_MAX_TIME;
	}

	EEPROM.put(EEPROM_STORAGE_COUNTDOWN, this->timeReset);
}

void DisplayTask_Countdown::handleButtonPress(Button button, PressType press) {
	if (DisplayTask::editMode) {
		DisplayTask::handleButtonPress(button, press);
		return;
	}

	if (press != Click && button != SET) {
		return;
	}

	switch (button) {
	case UP:
		this->resume();
		return;
	case DOWN:
		if (this->running) {
			this->pause();
		}
		else {
			this->reset();
		}
		return;
	}

	DisplayTask::handleButtonPress(button, press);
}

const bool DisplayTask_Countdown::_canShow() {
	return this->running || this->time != this->timeReset;
}

bool DisplayTask_Countdown::refresh(uint16_t displayData[]) {
	if (this->running) {
		DisplayTask::editMode = false;
		const unsigned long curMillis = millis();
		const unsigned long milliDiff = curMillis - this->lastCall;
		if (this->time > milliDiff) {
			this->time -= milliDiff;
		}
		else {
			this->time = 0;
		}
		this->lastCall = curMillis;

		if (this->time <= 0) {
			const uint16_t osym = (second() % 2) ? NO_TUBES : getNumber(0);
			for (byte i = 0; i < 6; i++) {
				displayData[i] = osym;
			}
			return this->running;
		}
	}

	return showShortTime(DisplayTask::editMode ? this->timeReset : this->time, !DisplayTask::editMode, displayData, DisplayTask::editMode) && DisplayTask::refresh(displayData);
}

void DisplayTask_Countdown::reset() {
	this->time = this->timeReset;
	this->running = false;
}

void DisplayTask_Countdown::pause() {
	this->running = false;
}

void DisplayTask_Countdown::resume() {
	if (!this->running) {
		this->lastCall = millis();
	}
	this->running = true;
}

void DisplayTask_Countdown::start() {
	this->time = this->timeReset;
	this->lastCall = millis();
	this->running = true;
}