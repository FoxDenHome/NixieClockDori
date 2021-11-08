#pragma once

#include "DisplayTask.h"

class DisplayTask_Date : public DisplayTask {
public:
	DisplayTask_Date();
	bool refresh() override;

	void loadConfig(const int16_t base);

	void handleEdit(const byte digit, const bool up) override;
	void handleButtonPress(const Button button, const PressType press) override;
protected:
	bool _canShow() const override;
private:
	bool cycleAuto;
	byte d, m, y;
	int16_t base = -1;
};
