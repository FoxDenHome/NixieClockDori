#include <Arduino.h>
#include <EEPROM.h>
#include <OneButton.h>

#include <avr/wdt.h>

#include "rtc.h"
#include "temperature.h"
#include "reset.h"
#include "config.h"
#include "const.h"
#include "serial.h"
#include "Display.h"
#include "DisplayDriver.h"
#include "DisplayTask.h"
#include "version.h"
#include "variables.h"

#ifdef ENABLE_ESPPROXY
#include "espproxy.h"
#endif

/**************************/
/* ARDUINO EVENT HANDLERS */
/**************************/

#define _DECL_BUTTON_FN(NAME, FUNC) \
	void __ ## NAME ## _BUTTON_ ## FUNC () { \
		DisplayTask::buttonHandler(NAME, FUNC); \
	}

#define _SETUP_BUTTON_FN(NAME, FUNC) \
	NAME ## Button.attach ## FUNC (__ ## NAME ## _BUTTON_ ## FUNC);

#define DECL_BUTTON(NAME) \
	OneButton NAME ## Button(PIN_BUTTON_ ## NAME, true); \
	_DECL_BUTTON_FN(NAME, Click) \
	_DECL_BUTTON_FN(NAME, LongPressStart)

#define SETUP_BUTTON(NAME) \
	NAME ## Button.setClickMs(200); \
	NAME ## Button.setPressMs(500); \
	_SETUP_BUTTON_FN(NAME, Click) \
	_SETUP_BUTTON_FN(NAME, LongPressStart)

DECL_BUTTON(DOWN)
DECL_BUTTON(UP)
DECL_BUTTON(SET)

void setup() {
	const uint8_t mcusr_mirror = MCUSR;
	MCUSR = 0;
	wdt_disable();

	// Pin setup
	pinMode(PIN_DISPLAY_LATCH, OUTPUT);
	digitalWrite(PIN_DISPLAY_LATCH, LOW);

	pinMode(PIN_LED_RED, OUTPUT);
	pinMode(PIN_LED_GREEN, OUTPUT);
	pinMode(PIN_LED_BLUE, OUTPUT);
	analogWrite(PIN_LED_RED, 1);
	analogWrite(PIN_LED_GREEN, 1);
	analogWrite(PIN_LED_BLUE, 1);

	pinMode(PIN_BUZZER, OUTPUT);

	pinMode(PIN_BUTTON_SET, INPUT_PULLUP);
	pinMode(PIN_BUTTON_UP, INPUT_PULLUP);
	pinMode(PIN_BUTTON_DOWN, INPUT_PULLUP);

	delay(100);

	// Begin initialization routines
	hostSerial.init();
	wifiSerial.init();
	rtcInit();
	displayInit();
	displayDriverInit();

	randomSeed(analogRead(A4) + now());

	DisplayEffect loadEffect;
	EEPROM.get(EEPROM_STORAGE_CURRENT_EFFECT, loadEffect);
	currentEffect = loadEffect;

	displayClock.loPri = true;
	displayClock.add();
	displayDate.loPri = true;
	displayDate.add();

	displayStopwatch.add();
	displayCountdown.add();

	displayTemp.loPri = true;
	displayTemp.add();

	displayClock.loadColor(EEPROM_STORAGE_CLOCK_RGB);
	displayDate.loadColor(EEPROM_STORAGE_DATE_RGB);
	displayDate.loadConfig(EEPROM_STORAGE_DATE_AUTO);
	displayStopwatch.loadColor(EEPROM_STORAGE_STOPWATCH_RGB);
	displayCountdown.loadColor(EEPROM_STORAGE_COUNTDOWN_RGB);
	displayCountdown.loadConfig(EEPROM_STORAGE_COUNTDOWN);
	displayTemp.loadColor(EEPROM_STORAGE_TEMPERATURE_RGB);

	DisplayTask::current = &displayClock;
	DisplayTask::current->isDirty = true;

	SETUP_BUTTON(UP);
	SETUP_BUTTON(DOWN);
	SETUP_BUTTON(SET);

	hostSerial.echoFirst(F("Ready "));
	hostSerial.sendEnd(String(mcusr_mirror));

	wdt_enable(WDTO_250MS);
}

void loop() {
	wdt_reset();

#ifdef ENABLE_ESPPROXY
	if (ESPPRoxyMode) {
        loopESPProxy();
		return;
    }
#endif

	UPButton.tick();
	DOWNButton.tick();
	SETButton.tick();

	displayLoop();
	displayDriverLoop();

	wifiSerial.loop();
	hostSerial.loop();
}
