#include "DisplayTask_Clock.h"
#include "Display.h"
#include "config.h"

#include <TimeLib.h>

bool DisplayTask_Clock::refresh(uint16_t displayData[]) {
	const time_t _n = now();
	const byte h = hour(_n);
	const byte m = minute(_n);
	const byte s = second(_n);

	if (!DisplayTask::editMode) {
		if (h < 4 && s != this->s && s % 5 == 2) {
			displayAntiPoison(1);
		}
		else if (m != this->m && m % 10 == 2) {
			displayAntiPoison(2);
		}
	}

	this->h = h;
	this->m = m;
	this->s = s;

	if (s % 2) {
		this->dotMask = makeDotMask(true, true);
	}
	else {
		this->dotMask = makeDotMask(false, false);
	}

#ifdef CLOCK_TRIM_HOURS
	insert1(0, h / 10, true, displayData);
	insert1(1, h, false, displayData);
#else
	insert2(0, h, false, displayData);
#endif
	insert2(2, m, false, displayData);
	insert2(4, s, false, displayData);

	return true;
}

