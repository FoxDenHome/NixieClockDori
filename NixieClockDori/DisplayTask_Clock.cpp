#include "DisplayTask_Clock.h"
#include "Display.h"
#include "config.h"

#include <TimeLib.h>

const bool DisplayTask_Clock::isLoPri() {
	return true;
}

bool DisplayTask_Clock::render(const Task* renderTask) {
	const time_t _n = now();
	const byte h = hour(_n);
	const byte s = second(_n);

	if (s % 2) {
		setDotsConst(true, true);
	}
	else {
		setDotsConst(false, false);
	}

#ifdef CLOCK_TRIM_HOURS
	insert1(0, h / 10, true);
	insert1(1, h, false);
#else
	insert2(0, h, false);
#endif
	insert2(2, minute(_n), false);
	insert2(4, s, false);

	if (h < 4 && s % 5 == 2) {
		displayAntiPoison(1);
		return false;
	}

	return true;
}

