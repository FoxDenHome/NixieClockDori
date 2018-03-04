#include "DisplayTask_Clock.h"
#include "Display.h"
#include "config.h"

#include <TimeLib.h>

bool DisplayTask_Clock::render(const unsigned long microDelta) {
	const time_t _n = now();
	const byte h = hour(_n);
	const byte s = second(_n);

	if (s % 2) {
		this->dotMask = makeDotMask(true, true);
	}
	else {
		this->dotMask = makeDotMask(false, false);
	}

#ifdef CLOCK_TRIM_HOURS
	insert1(0, h / 10, true, this->dataToDisplay);
	insert1(1, h, false, this->dataToDisplay);
#else
	insert2(0, h, false, this->dataToDisplay);
#endif
	insert2(2, minute(_n), false, this->dataToDisplay);
	insert2(4, s, false, this->dataToDisplay);

	if (h < 4 && s % 5 == 2) {
		displayAntiPoison(1);
	}

	return true;
}

