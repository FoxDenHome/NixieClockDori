#ifndef _DISPLAYTASK_DATE_H_INCLUDED
#define _DISPLAYTASK_DATE_H_INCLUDED

#include "DisplayTask.h"

class DisplayTask_Date : public DisplayTask {
public:
	DisplayTask_Date();
	bool refresh(uint16_t displayData[]) override;
	void handleEdit(byte digit, bool up) override;
	void handleButtonPress(Button button, PressType press) override;
protected:
	const bool _canShow() override;
private:
	bool cycleAuto;
	byte d, m, y;
};

#endif
