#pragma once

#define CLOCK_TICK_HALFSECOND

#include "DisplayTask.h"

class DisplayTask_Clock : public DisplayTask {
public:
	bool refresh() override;

	void handleEdit(const byte digit, const bool up) override;
	void handleButtonPress(const Button button, const PressType press) override;
private:
#ifdef CLOCK_TICK_HALFSECOND
	unsigned long lastSChange;
#endif
	byte h, m, s;
};
