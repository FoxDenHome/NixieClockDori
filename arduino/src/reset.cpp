#include "reset.h"
#include <avr/wdt.h>
#include <Arduino.h>

void forceReset() {
	MCUSR = 0;
	wdt_disable();

	// Get the watchdog stuck to force a reset!
	wdt_enable(WDTO_15MS);
	while (1) {
		delay(10);
	}
}
