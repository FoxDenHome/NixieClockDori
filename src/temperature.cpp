#include "const.h"
#include "config.h"
#include "temperature.h"

float curTemp = NAN;

void temperatureSet(float temp) {
	curTemp = temp;
}

float temperatureGet() {
	return curTemp;
}

int16_t temperatureGetInt() {
	if (isnan(curTemp)) {
		return INT16_MIN;
	}
	return (int16_t)round(curTemp);
}
