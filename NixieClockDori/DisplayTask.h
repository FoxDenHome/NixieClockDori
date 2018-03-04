#ifndef _DISPLAYTASK_H_INCLUDED
#define _DISPLAYTASK_H_INCLUDED

#include "Display.h"
#include <arduino.h>

class DisplayTask {
public:
	bool canShow();

	virtual bool refresh(uint16_t displayData[]) = 0;

	void add();
	void remove();

	static DisplayTask* findNextValid(DisplayTask *dt_current);

	byte red, green, blue;
	byte dotMask;

	bool loPri = false;
	bool removeOnCantShow = true;

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

