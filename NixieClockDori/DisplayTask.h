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

	virtual bool refresh(uint16_t displayData[]);
	virtual void handleButtonPress(Button button, PressType press);
	static void buttonHandler(Button button, PressType press);

	virtual void handleEdit(byte digit, bool up) { };

	void add();
	void remove();

	static DisplayTask* findNextValid(DisplayTask *dt_current, const bool mustCanShow);
	static void cycleDisplayUpdater();

	byte red, green, blue;
	byte dotMask;

	bool loPri = false;
	static bool editMode;

	static DisplayTask *current;

	static unsigned long nextDisplayCycleMicros;

protected:
	static unsigned long lastButtonPress;

	virtual const bool _canShow();
	bool removeOnCantShow = false;

	static byte editModePos;

	void _handleEditHelper(byte digit, bool up, byte& a, byte& b, byte& c, byte amax, byte bmax, byte cmax);

private:

	static DisplayTask* _findNextValid(DisplayTask *curPtr, DisplayTask *stopOn, const bool mustCanShow);
	DisplayTask *next;
	DisplayTask *prev;
	bool isAdded;
};

#endif

