#include "DisplayTask.h"
#include "Display.h"

DisplayTask *dt_first_lo;
DisplayTask *dt_last_lo;
DisplayTask *dt_first_hi;
DisplayTask *dt_last_hi;

DisplayTask* DisplayTask::current;

DisplayTask* DisplayTask::_findNextValid(DisplayTask *curPtr, DisplayTask *stopOn) {
	if (!curPtr) {
		return NULL;
	}

	do {
		if (curPtr == stopOn) {
			return NULL;
		}

		if (curPtr->canShow()) {
			return curPtr;
		}
	} while (curPtr = curPtr->next);

	return NULL;
}

DisplayTask* DisplayTask::findNextValid(DisplayTask *dt_current) {
	if (!dt_current) {
		if (dt_first_hi) {
			return DisplayTask::findNextValid(dt_first_hi);
		}
		else if (dt_first_lo) {
			return DisplayTask::findNextValid(dt_first_lo);
		}
		return NULL;
	}

	DisplayTask* curPtr;
	const bool isLoPri = dt_current->isLoPri();

	if (isLoPri) { // Search for first hi-pri
		curPtr = DisplayTask::_findNextValid(dt_first_hi, NULL);
		if (curPtr) {
			return curPtr;
		}
	}

	curPtr = DisplayTask::_findNextValid(dt_current->next, NULL);
	if (curPtr) {
		return curPtr;
	}

	curPtr = DisplayTask::_findNextValid(isLoPri ? dt_first_lo : dt_first_hi, dt_current);
	if (curPtr) {
		return curPtr;
	}

	if (dt_current->canShow()) {
		return dt_current;
	}

	if (!isLoPri) {
		return DisplayTask::_findNextValid(dt_first_lo, NULL);
	}

	return NULL;
}

const bool DisplayTask::isLoPri() {
	return false;
}

bool DisplayTask::canShow() {
	if (this->_canShow()) {
		return true;
	}
	if (this->_removeOnCantShow()) {
		this->remove();
	}
	return false;
}

const bool DisplayTask::_removeOnCantShow() {
	return true;
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

	if (this->isLoPri()) {
		if (dt_last_lo) {
			dt_last_lo->next = this;
			this->prev = dt_last_lo;
			dt_last_lo = this;
			return;
		}
		dt_first_lo = this;
		dt_last_lo = this;
	}
	else {
		if (dt_last_hi) {
			dt_last_hi->next = this;
			this->prev = dt_last_hi;
			dt_last_hi = this;
			return;
		}
		dt_first_hi = this;
		dt_last_hi = this;
	}
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

	if (this->isLoPri()) {
		if (this == dt_first_lo) {
			if (this->next) {
				dt_first_lo = this->next;
			}
			else {
				dt_first_lo = this->prev;
			}
		}
		if (this == dt_last_lo) {
			if (this->prev) {
				dt_last_lo = this->prev;
			}
			else {
				dt_last_lo = this->next;
			}
		}
	}
	else {
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
	}

	this->next = NULL;
	this->prev = NULL;
}

