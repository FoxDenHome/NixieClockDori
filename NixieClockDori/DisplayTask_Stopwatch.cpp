#include "DisplayTask_Stopwatch.h"
#include "Display.h"

const bool DisplayTask_Stopwatch::_canShow() {
	return this->time > 0;
}

bool DisplayTask_Stopwatch::refresh(uint16_t displayData[]) {
	if (this->running) {
		const unsigned long curMillis = millis();
		this->time += curMillis - this->lastCall;
		this->lastCall = curMillis;
	}

	return showShortTime(this->time, false, displayData, &this->dotMask);
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
	this->time = 1;
	this->lastCall = millis();
	this->running = true;
}

