#pragma once

#include "DisplayTask.h"

class DisplayTask_Date : public DisplayTask {
public:
	DisplayTask_Date();
	bool refresh(uint16_t displayData[]) override;

	void loadConfig(const int16_t base);

	void handleEdit(const byte digit, const bool up) override;
	void handleButtonPress(const Button button, const PressType press) override;
protected:
	const bool _canShow() override;
private:
	bool cycleAuto;
	byte d, m, y;
	int16_t base = -1;
};
