#pragma once

#include "DisplayTask.h"

class DisplayTask_Stopwatch : public DisplayTask {
public:
	DisplayTask_Stopwatch();

	bool refresh() override;

	void pause();
	void resume();
	void reset();
	void start();

	unsigned long getTime() const;

	void handleButtonPress(const Button button, const PressType press) override;

protected:
	bool isActive() const override;

private:
	bool running = false;
	unsigned long time = 0;
	unsigned long lastCall = 0;
};
