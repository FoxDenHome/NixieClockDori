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

	bool refresh() override;
	unsigned long timeReset;

	void handleButtonPress(const Button button, const PressType press) override;
	void handleEdit(const byte digit, const bool up) override;

protected:
	bool _canShow() const override;

private:
	bool running = false;
	unsigned long time = 0;
	unsigned long lastCall = 0;
	int16_t base = -1;
};
