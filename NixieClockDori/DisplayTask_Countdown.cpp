#include "DisplayTask_Countdown.h"
#include "Display.h"

#include <TimeLib.h>

DisplayTask_Countdown::DisplayTask_Countdown() {
	this->dotMask = makeDotMask(false, true);
	this->timeReset = 10000;
	this->reset();
}

void DisplayTask_Countdown::handleButtonPress(Button button, PressType press) {
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
	return this->running;
}

bool DisplayTask_Countdown::refresh(uint16_t displayData[]) {
	DisplayTask::editMode = false;

	if (this->running) {
		const unsigned long curMillis = millis();
		this->time -= curMillis - this->lastCall;
		this->lastCall = curMillis;
	}

	const unsigned long curMillis = millis();

	if (this->time <= 0) {
		const uint16_t osym = (second() % 2) ? NO_TUBES : getNumber(0);
		for (byte i = 0; i < 6; i++) {
			displayData[i] = osym;
		}
		return false;
	}

	return showShortTime(this->time, true, displayData);
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