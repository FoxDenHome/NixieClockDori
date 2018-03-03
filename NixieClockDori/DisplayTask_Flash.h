#ifndef _DISPLAYTASK_FLASH_H_INCLUDED
#define _DISPLAYTASK_FLASH_H_INCLUDED

#include "DisplayTask.h"

class DisplayTask_Flash : public DisplayTask {
public:
	bool render(const Task* renderTask) override;

	uint16_t symbols[6];
	byte dotsMask;
	unsigned long endTime;

protected:
	const bool _canShow() override;
};

#endif
