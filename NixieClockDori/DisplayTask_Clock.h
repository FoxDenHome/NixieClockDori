#pragma once

#include "DisplayTask.h"

class DisplayTask_Clock : public DisplayTask {
public:
	bool refresh(byte displayData[]) override;

	void handleEdit(const byte digit, const bool up) override;
	void handleButtonPress(const Button button, const PressType press) override;
private:
	byte h, m, s;
};
