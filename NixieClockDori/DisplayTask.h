#ifndef _DISPLAYTASK_H_INCLUDED
#define _DISPLAYTASK_H_INCLUDED

#include "Display.h"
#include <arduino.h>

enum Button {
	SET,
	UP,
	DOWN,
};
enum PressType {
	Click,
	LongPressStart,
};

class DisplayTask {
public:
	bool canShow();

	virtual bool refresh(uint16_t displayData[]) = 0;
	virtual void handleButtonPress(Button button, PressType press);
	static void buttonHandler(Button button, PressType press);

	void add();
	void remove();

	static DisplayTask* findNextValid(DisplayTask *dt_current, const boolean mustCanShow);
	static void cycleDisplayUpdater();

	byte red, green, blue;
	byte dotMask;

	bool loPri = false;
	static bool editMode;

	static DisplayTask *current;

	static unsigned long nextDisplayCycleMicros;

protected:
	virtual const bool _canShow();
	bool removeOnCantShow = false;

	static byte editModePos;

private:
	static DisplayTask* _findNextValid(DisplayTask *curPtr, DisplayTask *stopOn, const boolean mustCanShow);
	DisplayTask *next;
	DisplayTask *prev;
	bool isAdded;
};

#endif

