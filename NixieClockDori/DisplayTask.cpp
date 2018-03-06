#include "DisplayTask.h"
#include "Display.h"

DisplayTask *dt_first_hi;
DisplayTask *dt_last_hi;

DisplayTask* DisplayTask::current;

unsigned long DisplayTask::nextDisplayCycleMicros = 0;
bool DisplayTask::editMode = false;
byte DisplayTask::editModePos = 0;
unsigned long DisplayTask::lastButtonPress = 0;

void DisplayTask::cycleDisplayUpdater() {
	DisplayTask::nextDisplayCycleMicros = micros() + DISPLAY_CYCLE_PERIOD;

	if (DisplayTask::editMode || (DisplayTask::current && !DisplayTask::current->loPri && DisplayTask::current->canShow())) {
		return;
	}
	DisplayTask::current = DisplayTask::findNextValid(DisplayTask::current, true);
}

void DisplayTask::buttonHandler(Button button, PressType pressType) {
	DisplayTask::nextDisplayCycleMicros = micros() + DISPLAY_CYCLE_PERIOD;
	displayAntiPoisonOff();
	DisplayTask::lastButtonPress = millis();

	if (!DisplayTask::current) {
		return;
	}
	DisplayTask::current->handleButtonPress(button, pressType);
}

bool DisplayTask::refresh(uint16_t displayData[]) {
	if (this->editMode) {
		if ((millis() - DisplayTask::lastButtonPress) % 1000 >= 500) {
			displayData[this->editModePos] = NO_TUBES;
		}
		return false;
	}
	return true;
}

void DisplayTask::handleButtonPress(Button button, PressType pressType) {
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

DisplayTask* DisplayTask::_findNextValid(DisplayTask *curPtr, DisplayTask *stopOn, const bool mustCanShow) {
	if (!curPtr) {
		return NULL;
	}

	do {
		if (curPtr == stopOn) {
			return NULL;
		}

		// Trigger canShow to allow self-remove, then check if added if not must can show
		if (curPtr->canShow() || (!mustCanShow && curPtr->isAdded)) {
			return curPtr;
		}
	} while ((curPtr = curPtr->next));

	return NULL;
}

DisplayTask* DisplayTask::findNextValid(DisplayTask *dt_current, const bool mustCanShow) {
	if (!dt_current) {
		if (dt_first_hi) {
			return DisplayTask::findNextValid(dt_first_hi, mustCanShow);
		}
		return NULL;
	}

	DisplayTask* curPtr;

	curPtr = DisplayTask::_findNextValid(dt_current->next, NULL, mustCanShow);
	if (curPtr) {
		return curPtr;
	}

	curPtr = DisplayTask::_findNextValid(dt_first_hi, dt_current, mustCanShow);
	if (curPtr) {
		return curPtr;
	}

	if (dt_current->canShow() || (!mustCanShow && dt_current->isAdded)) {
		return dt_current;
	}

	return NULL;
}

bool DisplayTask::canShow() {
	if (this->_canShow()) {
		return true;
	}
	if (this->removeOnCantShow) {
		this->remove();
	}
	return false;
}


const bool DisplayTask::_canShow() {
	return true;
}

void DisplayTask::add() {
	if (this->isAdded) {
		return;
	}
	this->isAdded = true;

	this->next = NULL;

	if (dt_last_hi) {
		dt_last_hi->next = this;
		this->prev = dt_last_hi;
		dt_last_hi = this;
		return;
	}
	dt_first_hi = this;
	dt_last_hi = this;
}

void DisplayTask::remove() {
	if (!this->isAdded) {
		return;
	}
	this->isAdded = false;

	if (this->next) {
		this->next->prev = this->prev;
	}

	if (this->prev) {
		this->prev->next = this->next;
	}

	if (this == dt_first_hi) {
		if (this->next) {
			dt_first_hi = this->next;
		}
		else {
			dt_first_hi = this->prev;
		}
	}
	if (this == dt_last_hi) {
		if (this->prev) {
			dt_last_hi = this->prev;
		}
		else {
			dt_last_hi = this->next;
		}
	}

	this->next = NULL;
	this->prev = NULL;
}

