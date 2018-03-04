#ifndef _DISPLAYTASK_CLOCK_H_INCLUDED
#define _DISPLAYTASK_CLOCK_H_INCLUDED

#include "DisplayTask.h"

class DisplayTask_Clock : public DisplayTask {
public:
	bool refresh(uint16_t displayData[]) override;
private:
	byte h, m, s;
};

#endif

