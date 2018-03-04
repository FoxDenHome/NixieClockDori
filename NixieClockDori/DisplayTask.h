#ifndef _DISPLAYTASK_H_INCLUDED
#define _DISPLAYTASK_H_INCLUDED

#include "Display.h"
#include <arduino.h>

class DisplayTask {
public:
	bool canShow();

	// Returns true if change-effects should show
	virtual bool render() = 0;

	void add();
	void remove();

	static DisplayTask* findNextValid(DisplayTask *dt_current);

	byte red, green, blue;
	uint16_t dataToDisplay[6] = { NO_TUBES, NO_TUBES, NO_TUBES, NO_TUBES, NO_TUBES };
	byte dotMask;

	bool loPri = false;
	bool removeOnCantShow = true;
	unsigned long renderPeriodMicros = 50000;
	unsigned long nextRender = 0;

	static DisplayTask *current;

protected:
	virtual const bool _canShow();

private:
	static DisplayTask* _findNextValid(DisplayTask *curPtr, DisplayTask *stopOn);
	DisplayTask *next;
	DisplayTask *prev;
	bool isAdded;
};

#endif

