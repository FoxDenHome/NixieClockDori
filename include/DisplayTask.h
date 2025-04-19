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
	bool isActive();

	virtual bool refresh();
	virtual void handleButtonPress(const Button button, const PressType press);
	static void buttonHandler(const Button button, const PressType press);

	virtual void handleEdit(const byte /*digit*/, const bool /*up*/) { };

	void add();
	void remove();

	static DisplayTask* findNextValid(DisplayTask *dt_current, const bool mustIsActive);
	static void cycleDisplayUpdater();
	void showIfPossibleOtherwiseRotateIfCurrent();

	byte red = 0, green = 0, blue = 0;
	byte dotMask = 0;
	uint16_t displayData[9];
	bool isDirty = false;

	bool loPri = false;
	static bool editMode;
	static bool buttonLock;

	static DisplayTask *current;

	static unsigned long lastDisplayCycleMicros;

	void saveColor(int16_t addr);
	void loadColor(int16_t addr);
	void setDisplayData(const byte offset, const uint16_t data);
	void setColorFromInput(const byte offset, const int16_t eepromBase, const String& data);

protected:
	static unsigned long lastButtonPress;

	virtual bool isStackable() const;
	virtual bool _isActive() const;
	bool removeOnCantShow = false;

	static byte editModePos;

	void _handleEditHelper(const byte digit, const bool up, byte& a, byte& b, byte& c, const byte amax, const byte bmax, const byte cmax);

	bool insertTemp();
	float lastTemp = NAN;

	void insert1(const byte offset, const byte data, const bool trimLeadingZero);
	bool insert2(const byte offset, const byte data, const bool trimLeadingZero);
	bool showShortTime(const unsigned long timeMs, bool trimLZ);

	void addToStack();

private:
	static DisplayTask* _findNextValid(DisplayTask *curPtr, DisplayTask *stopOn, const bool mustIsActive);

	DisplayTask *list_next;
	DisplayTask *list_prev;

	DisplayTask *stack_prev;

	bool isAdded = false;
};
