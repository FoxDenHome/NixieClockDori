#include <EEPROM.h>
#include <DS3232RTC.h>

#include "DisplayTask.h"
#include "Display.h"
#include "reset.h"
#include "temperature.h"
#include "const.h"
#include "serial.h"
#include "utils.h"

DisplayTask *dt_first;
DisplayTask *dt_last;

DisplayTask* DisplayTask::current;

bool DisplayTask::buttonLock = false;

unsigned long DisplayTask::lastDisplayCycleMicros = 0;
bool DisplayTask::editMode = false;
byte DisplayTask::editModePos = 0;
unsigned long DisplayTask::lastButtonPress = 0;

void DisplayTask::saveColor(const int16_t addr) {
	if (addr < 0) {
		return;
	}

	EEPROM.put(addr, this->red);
	EEPROM.put(addr + 1, this->green);
	EEPROM.put(addr + 2, this->blue);
}

void DisplayTask::loadColor(const int16_t addr) {
	if (addr < 0) {
		return;
	}

	EEPROM.get(addr, this->red);
	EEPROM.get(addr + 1, this->green);
	EEPROM.get(addr + 2, this->blue);
}

void DisplayTask::cycleDisplayUpdater() {
	DisplayTask::lastDisplayCycleMicros = micros();

	if (DisplayTask::editMode || (!DisplayTask::current->loPri && DisplayTask::current->isActive())) {
		return;
	}

	DisplayTask::current = DisplayTask::findNextValid(DisplayTask::current, true);
	DisplayTask::current->isDirty = true;
}

void DisplayTask::addToStack() {
	if (DisplayTask::current != this) {
		this->stack_prev = DisplayTask::current;
	}
}

void DisplayTask::showIfPossibleOtherwiseRotateIfCurrent() {
	if (this->isActive()) {
		this->add();
		DisplayTask::editMode = false;
		DisplayTask::current = this;
		this->isDirty = true;
		DisplayTask::lastDisplayCycleMicros = micros();
	}
	else if (this == DisplayTask::current) {
		DisplayTask::cycleDisplayUpdater();
	}
	else {
		return;
	}
	displayAntiPoisonOff();
}

void DisplayTask::buttonHandler(const Button button, const PressType pressType) {
	if (DisplayTask::buttonLock) {
		DisplayTask::current->editMode = false;
		DisplayTask::current->editModePos = 0;
		return;
	}

	DisplayTask::lastDisplayCycleMicros = micros();
	displayAntiPoisonOff();
	DisplayTask::lastButtonPress = millis();

	DisplayTask::current->handleButtonPress(button, pressType);
}

bool DisplayTask::refresh() {
	if (this->editMode) {
		if ((millis() - DisplayTask::lastButtonPress) % 1000 >= 500) {
			insert1(this->editModePos, 0, true);
		}
		return false;
	}
	return true;
}

bool DisplayTask::insertTemp() {
	const int16_t temp = temperatureGetInt();
	if (temp == this->lastTemp) {
		return false;
	}

	if (temp == INT16_MIN) {
		this->setDisplayData(6, NO_TUBES);
		this->setDisplayData(7, NO_TUBES);
		this->setDisplayData(8, NO_TUBES);
	} else {
		const int16_t absTemp = abs(temp);
		if (temp < 0) {
			this->dotMask |= DOT_3_UP;
		}

		if (absTemp >= 10) {
			this->setDisplayData(6, getNumber(absTemp / 10));
		} else {
			this->setDisplayData(6, NO_TUBES);
		}
		this->setDisplayData(7, getNumber(absTemp + 0.5));
		this->setDisplayData(8, SYMBOL_DEGREES_C);
	}

	return true;
}

void DisplayTask::setDisplayData(const byte offset, const uint16_t data) {
	if (this->displayData[offset] == data) {
		return;
	}
	this->displayData[offset] = data;
	this->isDirty = true;
}

void DisplayTask::insert1(const byte offset, const byte data, const bool trimLeadingZero) {
	if (data == 0 && trimLeadingZero) {
		this->setDisplayData(offset, NO_TUBES);
	}
	else {
		this->setDisplayData(offset, getNumber(data));
	}
}

bool DisplayTask::insert2(const byte offset, const byte data, const bool trimLeadingZero) {
	this->insert1(offset, data / 10, trimLeadingZero);
	this->insert1(offset + 1, data, trimLeadingZero);
	return data == 0 && trimLeadingZero;
}


bool DisplayTask::showShortTime(const unsigned long timeMs, bool trimLZ) {
	trimLZ = this->insert2(0, (timeMs / ONE_HOUR_IN_MS) % 100, trimLZ);
	trimLZ = this->insert2(2, (timeMs / ONE_MINUTE_IN_MS) % 60, trimLZ);
	trimLZ = this->insert2(4, (timeMs / ONE_SECOND_IN_MS) % 60, trimLZ);
	this->insert2(6, (timeMs / 10UL) % 100, trimLZ);
	return false;
}

void DisplayTask::setColorFromInput(const byte offset, const int16_t eepromBase, const String& data) {
	if (data.length() < (unsigned int)offset + 6) {
		return;
	}
	this->red = hexInputToByte(offset, data);
	this->green = hexInputToByte(offset + 2, data);
	this->blue = hexInputToByte(offset + 4, data);
	this->saveColor(eepromBase);
}

void __handleEditHelperSingle(const byte digit, const bool up, byte& a, const byte amax) {
	const byte a1 = a % 10;
	const byte amax1 = amax % 10;
	const byte amax10 = amax - amax1;

	switch (digit % 2) {
	case 0:
		if (up) {
			a += 10;
			if (a > amax) {
				a = a1;
			}
		}
		else {
			if (a < 10) {
				a = a1 + amax10;
				if (a > amax) {
					a -= 10;
				}
			}
			else {
				a -= 10;
			}
		}
		break;
	case 1:
		if (up) {
			if (a1 == 9 || a >= amax) {
				a -= a1;
			}
			else {
				a += 1;
			}
		}
		else {
			if (a1 == 0) {
				if (a >= amax10) {
					a += amax1;
				}
				else {
					a += 9;
				}
			}
			else {
				a -= 1;
			}
		}
		break;
	}
}

void DisplayTask::_handleEditHelper(const byte digit, const bool up, byte& a, byte& b, byte& c, const byte amax, const byte bmax, const byte cmax) {
	switch (digit) {
	case 0:
	case 1:
		__handleEditHelperSingle(digit, up, a, amax);
		break;
	case 2:
	case 3:
		__handleEditHelperSingle(digit, up, b, bmax);
		break;
	case 4:
	case 5:
		__handleEditHelperSingle(digit, up, c, cmax);
		break;
	}
}

void DisplayTask::handleButtonPress(const Button button, const PressType pressType) {
	switch (button) {
	case SET:
		switch (pressType) {
		case Click:
			if (DisplayTask::editMode) {
				if (++DisplayTask::editModePos > 5) {
					DisplayTask::editModePos = 0;
				}
			}
			else {
				DisplayTask::current = DisplayTask::findNextValid(DisplayTask::current, false);
				DisplayTask::current->isDirty = true;
			}
			break;
		case LongPressStart:
			if (DisplayTask::editMode) {
				handleEdit(255, false);
			}
			DisplayTask::editMode = !DisplayTask::editMode;
			DisplayTask::editModePos = 0;
			break;
		}
		break;
	case DOWN:
	case UP:
		if (this->editMode) {
			handleEdit(DisplayTask::editModePos, button == UP);
		}
		break;
	}
}

DisplayTask* DisplayTask::_findNextValid(DisplayTask *curPtr, DisplayTask *stopOn, const bool mustIsActive) {
	if (!curPtr) {
		return NULL;
	}

	do {
		if (curPtr == stopOn) {
			return NULL;
		}

		// Trigger isActive to allow self-remove, then check if added if not must can show
		if (curPtr->isActive() || (!mustIsActive && curPtr->isAdded)) {
			return curPtr;
		}
	} while ((curPtr = curPtr->list_next));

	return NULL;
}

DisplayTask* DisplayTask::findNextValid(DisplayTask *dt_current, const bool mustIsActive) {
	if (!dt_current) {
		if (dt_first) {
			return DisplayTask::findNextValid(dt_first, mustIsActive);
		}

		forceReset();
		return NULL;
	}

	DisplayTask* curPtr;

	if (dt_current->stack_prev) {
		curPtr = dt_current->stack_prev;
		dt_current->stack_prev = NULL;
		return curPtr;
	}

	curPtr = DisplayTask::_findNextValid(dt_current->list_next, NULL, mustIsActive);
	if (curPtr) {
		return curPtr;
	}

	curPtr = DisplayTask::_findNextValid(dt_first, dt_current, mustIsActive);
	if (curPtr) {
		return curPtr;
	}

	if (dt_current->isActive() || (!mustIsActive && dt_current->isAdded)) {
		return dt_current;
	}

	forceReset();
	return NULL;
}

bool DisplayTask::isActive() {
	if (this->_isActive()) {
		return true;
	}
	if (this->removeOnCantShow) {
		this->remove();
	}
	return false;
}


bool DisplayTask::_isActive() const {
	return true;
}

void DisplayTask::add() {
	if (this->isAdded) {
		return;
	}
	this->isAdded = true;

	this->list_next = NULL;

	if (dt_last) {
		dt_last->list_next = this;
		this->list_prev = dt_last;
		dt_last = this;
		return;
	}
	dt_first = this;
	dt_last = this;
}

void DisplayTask::remove() {
	if (!this->isAdded) {
		return;
	}
	this->isAdded = false;

	if (this->list_next) {
		this->list_next->list_prev = this->list_prev;
	}

	if (this->list_prev) {
		this->list_prev->list_next = this->list_next;
	}

	if (this == dt_first) {
		if (this->list_next) {
			dt_first = this->list_next;
		}
		else {
			dt_first = this->list_prev;
		}
	}
	if (this == dt_last) {
		if (this->list_prev) {
			dt_last = this->list_prev;
		}
		else {
			dt_last = this->list_next;
		}
	}

	this->list_next = NULL;
	this->list_prev = NULL;
}

