#pragma once

#include "Display.h"

#include <Arduino.h>

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

	virtual bool refresh();
	virtual void handleButtonPress(const Button button, const PressType press);
	static void buttonHandler(const Button button, const PressType press);

	virtual void handleEdit(const byte /*digit*/, const bool /*up*/) { };

	void add();
	void remove();

	static DisplayTask* findNextValid(DisplayTask *dt_current, const bool mustCanShow);
	static void cycleDisplayUpdater();

	byte red = 0, green = 0, blue = 0;
	byte dotMask = 0;

	bool loPri = false;
	static bool editMode;

	static DisplayTask *current;

	static unsigned long lastDisplayCycleMicros;

	void saveColor(int16_t addr);
	void loadColor(int16_t addr);

protected:
	static unsigned long lastButtonPress;

	virtual bool _canShow() const;
	bool removeOnCantShow = false;

	static byte editModePos;

	void _handleEditHelper(const byte digit, const bool up, byte& a, byte& b, byte& c, const byte amax, const byte bmax, const byte cmax);

private:

	static DisplayTask* _findNextValid(DisplayTask *curPtr, DisplayTask *stopOn, const bool mustCanShow);
	DisplayTask *next;
	DisplayTask *prev;
	bool isAdded = false;
};
