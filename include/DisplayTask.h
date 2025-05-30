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
	virtual bool refresh();
	virtual void handleButtonPress(const Button button, const PressType press);
	static void buttonHandler(const Button button, const PressType press);

	virtual void handleEdit(const byte /*digit*/, const bool /*up*/) { };

	// Add/Remove from selection cycle
	void add();
	void remove();

	static DisplayTask* findNext(DisplayTask *dt_current);
	void showIfActiveOtherwiseShowSelected();

	byte red = 0, green = 0, blue = 0;
	byte dotMask = 0;
	uint16_t displayData[9];
	bool isDirty = false;

	bool loPri = false;
	static bool editMode;
	static bool buttonLock;

	void saveColor(int16_t addr);
	void loadColor(int16_t addr);
	void setDisplayData(const byte offset, const uint16_t data);
	void setColorFromInput(const byte offset, const int16_t eepromBase, const String& data);
	void select();
	bool isSelected() const;

	void setCurrent();

	static DisplayTask* retrieveCurrent();
	static void activateSelected();

	virtual bool isActive() const;

	static DisplayTask *standard;

protected:
	static unsigned long lastButtonPress;

	static byte editModePos;

	void _handleEditHelper(const byte digit, const bool up, byte& a, byte& b, byte& c, const byte amax, const byte bmax, const byte cmax);

	bool insertTemp();
	float lastTemp = NAN;

	void insert1(const byte offset, const byte data, const bool trimLeadingZero);
	bool insert2(const byte offset, const byte data, const bool trimLeadingZero);
	bool showShortTime(const unsigned long timeMs, bool trimLZ);

private:
	DisplayTask *list_next;
	DisplayTask *list_prev;

	static DisplayTask *current;
	static DisplayTask *selected;

	bool isAdded = false;
};
