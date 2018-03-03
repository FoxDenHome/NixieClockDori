#include "DisplayTask_Stopwatch.h"
#include "Display.h"

const bool DisplayTask_Stopwatch::_canShow() {
	return this->time > 0;
}

bool DisplayTask_Stopwatch::render(const unsigned long microDelta, uint16_t dataToDisplay[], byte *dotMask) {
	if (this->running) {
		this->time += microDelta / 1000UL;
	}

	return showShortTime(this->time, true, dataToDisplay, dotMask);
}

void DisplayTask_Stopwatch::reset() {
	this->time = 0;
	this->running = false;
}

void DisplayTask_Stopwatch::pause() {
	this->running = false;
}

void DisplayTask_Stopwatch::resume() {
	this->running = true;
}

void DisplayTask_Stopwatch::start() {
	this->time = 1;
	this->running = true;
}

