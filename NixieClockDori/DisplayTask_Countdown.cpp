#include "DisplayTask_Countdown.h"
#include "Display.h"
#include "const.h"

#include <EEPROM.h>
#include <TimeLib.h>

#define COUNTDOWN_MAX_TIME (100UL * 60UL * 60UL * 1000UL)

DisplayTask_Countdown::DisplayTask_Countdown() {
	this->dotMask = DOT_1_UP | DOT_2_UP | DOT_3_DOWN;
}

void DisplayTask_Countdown::loadConfig(const int16_t base) {
	this->base = base;
	if (base < 0) {
		return;
	}

	EEPROM.get(base, this->timeReset);
	if (!this->timeReset || this->timeReset >= COUNTDOWN_MAX_TIME) {
		this->timeReset = 10000;
	}
	this->reset();
}

void DisplayTask_Countdown::handleEdit(const byte digit, const bool up) {
	byte h = this->timeReset / (60UL * 60UL * 1000UL);
	byte m = (this->timeReset / (60UL * 1000UL)) % 60;
	byte s = (this->timeReset / 1000UL) % 60;

	this->_handleEditHelper(digit, up, h, m, s, 99, 59, 59);
	
	this->timeReset = ((((h * 60UL) + m) * 60UL) + s) * 1000UL;

	if (this->timeReset >= COUNTDOWN_MAX_TIME) {
		this->timeReset %= COUNTDOWN_MAX_TIME;
	}

	if (digit == 255) {
		this->time = this->timeReset;
		if (this->base >= 0) {
			EEPROM.put(this->base, this->timeReset);
		}
	}
}

void DisplayTask_Countdown::handleButtonPress(const Button button, const PressType press) {
	if (DisplayTask::editMode) {
		DisplayTask::handleButtonPress(button, press);
		return;
	}

	if (press != Click && button != SET) {
		return;
	}

	switch (button) {
	case SET:
		break;
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

bool DisplayTask_Countdown::_canShow() const {
	return this->running || this->time != this->timeReset;
}

bool DisplayTask_Countdown::refresh() {
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
			const byte osym = (second() % 2) ? NO_TUBES : getNumber(0);
			for (byte i = 0; i < 8; i++) {
				this->setDisplayData(i, osym);
			}
			return this->running;
		}
	}

	return (showShortTime(DisplayTask::editMode ? this->timeReset : this->time, !DisplayTask::editMode, DisplayTask::editMode) || !this->running) && DisplayTask::refresh();
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