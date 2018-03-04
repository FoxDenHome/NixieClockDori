#ifndef _DISPLAYTASK_CLOCK_H_INCLUDED
#define _DISPLAYTASK_CLOCK_H_INCLUDED

#include "DisplayTask.h"

class DisplayTask_Clock : public DisplayTask {
public:
	bool render() override;
};

#endif

