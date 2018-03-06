#ifndef _DISPLAYTASK_CLOCK_H_INCLUDED
#define _DISPLAYTASK_CLOCK_H_INCLUDED

#include "DisplayTask.h"

class DisplayTask_Clock : public DisplayTask {
public:
	bool refresh(uint16_t displayData[]) override;

	void handleEdit(const byte digit, const bool up) override;
	void handleButtonPress(const Button button, const PressType press) override;
private:
	byte h, m, s;
};

#endif

