#pragma once

#include "DisplayTask.h"

class DisplayTask_Countdown : public DisplayTask {
public:
	DisplayTask_Countdown();

	void loadConfig(const int16_t base);

	void reset();
	void pause();
	void resume();
	void start();

	bool refresh(uint16_t displayData[]) override;
	unsigned long timeReset;

	void handleButtonPress(const Button button, const PressType press) override;
	void handleEdit(const byte digit, const bool up) override;

protected:
	const bool _canShow() override;

private:
	bool running;
	unsigned long time;
	unsigned long lastCall;
	int16_t base;
};
