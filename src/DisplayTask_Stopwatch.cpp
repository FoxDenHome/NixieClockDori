#include "DisplayTask_Stopwatch.h"
#include "Display.h"

DisplayTask_Stopwatch::DisplayTask_Stopwatch() {
	this->dotMask = DOT_1_UP | DOT_1_DOWN | DOT_2_UP | DOT_2_DOWN | DOT_3_DOWN;
	this->reset();
}

void DisplayTask_Stopwatch::handleButtonPress(const Button button, const PressType press) {
	if (press != Click) {
		return;
	}

	switch (button) {
	case SET:
		break;
	case UP:
		if (!this->running) {
			this->reset();
		}
		return;
	case DOWN:
		if (this->running) {
			this->pause();
		}
		else {
			this->resume();
		}
		return;
	}

	DisplayTask::handleButtonPress(button, press);
}

bool DisplayTask_Stopwatch::_isActive() const {
	return this->time > 0 || this->running;
}

bool DisplayTask_Stopwatch::refresh() {
	DisplayTask::editMode = false;

	if (this->running) {
		const unsigned long curMillis = millis();
		this->time += curMillis - this->lastCall;
		this->lastCall = curMillis;
	}

	return showShortTime(this->time, false) || !this->running;
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

unsigned long DisplayTask_Stopwatch::getTime() const {
	return this->time;
}
