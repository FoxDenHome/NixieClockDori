#pragma once

#include "DisplayTask.h"

class DisplayTask_Stopwatch : public DisplayTask {
public:
	DisplayTask_Stopwatch();

	bool refresh(byte displayData[]) override;

	void pause();
	void resume();
	void reset();
	void start();

	void handleButtonPress(const Button button, const PressType press) override;

protected:
	const bool _canShow() override;

private:
	bool running = false;
	unsigned long time = 0;
	unsigned long lastCall = 0;
};
