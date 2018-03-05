#include "DisplayTask_Stopwatch.h"
#include "Display.h"

DisplayTask_Stopwatch::DisplayTask_Stopwatch() {
	this->dotMask = makeDotMask(true, false);
	this->reset();
}

void DisplayTask_Stopwatch::handleButtonPress(Button button, PressType press) {
	if (press != Click) {
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

const bool DisplayTask_Stopwatch::_canShow() {
	return this->time > 0 || this->running;
}

bool DisplayTask_Stopwatch::refresh(uint16_t displayData[]) {
	DisplayTask::editMode = false;

	if (this->running) {
		const unsigned long curMillis = millis();
		this->time += curMillis - this->lastCall;
		this->lastCall = curMillis;
	}

	return showShortTime(this->time, false, displayData);
}

void DisplayTask_Stopwatch::reset() {
	this->time = 0;
	this->running = false;
}

void DisplayTask_Stopwatch::pause() {
	this->running = false;
}

void DisplayTask_Stopwatch::resume() {
	if (!this->running) {
		this->lastCall = millis();
	}
	this->running = true;
}

void DisplayTask_Stopwatch::start() {
	this->time = 0;
	this->lastCall = millis();
	this->running = true;
}

