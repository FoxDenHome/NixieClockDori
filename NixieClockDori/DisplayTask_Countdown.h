#ifndef _DISPLAYTASK_COUNTDOWN_H_INCLUDED
#define _DISPLAYTASK_COUNTDOWN_H_INCLUDED

#include "DisplayTask.h"

class DisplayTask_Countdown : public DisplayTask {
public:
	DisplayTask_Countdown();

	void reset();
	void pause();
	void resume();
	void start();

	bool refresh(uint16_t displayData[]) override;
	unsigned long timeReset;

	void handleButtonPress(Button button, PressType press) override;
	void handleEdit(byte digit, bool up) override;

protected:
	const bool _canShow() override;

private:
	bool running;
	unsigned long time;
	unsigned long lastCall;
};

#endif

