#include <OneWire.h>
#include <DallasTemperature.h>

#include "const.h"
#include "config.h"
#include "temperature.h"

OneWire temperatureOneWire = OneWire(PIN_TEMPERATURE);
DallasTemperature temperatureSensor(&temperatureOneWire);

float curTemp = 0;
bool isInGet = false;
unsigned long lastRun = 0;

void temperatureInit() {
	temperatureSensor.begin();
	temperatureSensor.setWaitForConversion(false);

	temperatureSensor.requestTemperatures();
	lastRun = millis();
	isInGet = true;
}

void temperatureLoop() {
	const unsigned long curMillis = millis();
	const unsigned long timeSinceLast = curMillis - lastRun;

	if (isInGet && timeSinceLast >= 1000UL) {
		curTemp = temperatureSensor.getTempCByIndex(0);
		lastRun = curMillis;
		isInGet = false;
	} else if (!isInGet && timeSinceLast >= 30000UL) {
		temperatureSensor.requestTemperatures();
		lastRun = curMillis;
		isInGet = true;
	}
}

float temperatureGet() {
	return curTemp;
}
