#include "DisplayTask_Clock.h"
#include "Display.h"
#include "config.h"

#include <TimeLib.h>

const bool DisplayTask_Clock::isLoPri() {
	return true;
}

bool DisplayTask_Clock::render(const unsigned long microDelta, uint16_t dataToDisplay[], byte *dotMask) {
	const time_t _n = now();
	const byte h = hour(_n);
	const byte s = second(_n);

	if (s % 2) {
		*dotMask = makeDotMask(true, true);
	}
	else
		*dotMask = makeDotMask(false, false); {
	}

#ifdef CLOCK_TRIM_HOURS
	insert1(0, h / 10, true, dataToDisplay);
	insert1(1, h, false, dataToDisplay);
#else
	insert2(0, h, false, dataToDisplay);
#endif
	insert2(2, minute(_n), false, dataToDisplay);
	insert2(4, s, false, dataToDisplay);

	if (h < 4 && s % 5 == 2) {
		displayAntiPoison(1);
		return false;
	}

	return true;
}

