#ifndef _DISPLAYTASK_H_INCLUDED
#define _DISPLAYTASK_H_INCLUDED

#include <SoftTimer.h>
#include <arduino.h>

class DisplayTask {
public:
	bool canShow();
	virtual const bool isLoPri();

	// Returns true if change-effects should show
	virtual bool render(const unsigned long microDelta, uint16_t dataToDisplay[], byte *dotMask) = 0;

	void add();
	void remove();

	static DisplayTask* findNextValid(DisplayTask *dt_current);

	byte red, green, blue;
	static DisplayTask *current;

protected:
	virtual const bool _removeOnCantShow();
	virtual const bool _canShow();

private:
	static DisplayTask* _findNextValid(DisplayTask *curPtr, DisplayTask *stopOn);
	DisplayTask *next;
	DisplayTask *prev;
	bool isAdded;
};

#endif

