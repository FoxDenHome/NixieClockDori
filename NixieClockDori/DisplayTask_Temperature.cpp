#include "DisplayTask_Temperature.h"
#include "Display.h"
#include "temperature.h"

DisplayTask_Temperature::DisplayTask_Temperature() {
	this->dotMask = DOT_2_DOWN;
}

bool DisplayTask_Temperature::_canShow() const {
	return false;
}

bool DisplayTask_Temperature::refresh() {
	DisplayTask::editMode = false;

	const float temp = temperatureGet();

	const bool hideNext = insert2(0, temp / 100.0, true);
	insert2(2, temp, hideNext);
	insert2(4, temp * 100.0, false);
	insert2(6, temp * 10000.0, false);

	displayData[4] = SYMBOL_DEGREES_C;

	return true;
}
