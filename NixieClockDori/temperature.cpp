#include <OneWire.h>
#include <DallasTemperature.h>
#include <avr/wdt.h>

#include "const.h"
#include "config.h"
#include "temperature.h"

OneWire temperatureOneWire = OneWire(PIN_TEMPERATURE);
DallasTemperature temperatureSensor(&temperatureOneWire);

float curTemp = 0;
bool isInGet = false;

void temperatureInit() {
	temperatureSensor.begin();
	temperatureSensor.setWaitForConversion(false);
}

void temperatureLoop() {
	static unsigned long lastRun = 0;
	const unsigned long curMillis = millis();

	const unsigned long timeSinceLast = curMillis - lastRun;

	if (isInGet && timeSinceLast >= 1000UL) {
		curTemp = temperatureSensor.getTempCByIndex(0);
		lastRun = curMillis;
		isInGet = false;
	} else if (!isInGet && timeSinceLast >= 5000UL) {
		temperatureSensor.requestTemperatures();
		lastRun = curMillis;
		isInGet = true;
	}
}

float temperatureGet() {
	return curTemp;
}
