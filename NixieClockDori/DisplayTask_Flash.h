#ifndef _DISPLAYTASK_FLASH_H_INCLUDED
#define _DISPLAYTASK_FLASH_H_INCLUDED

#include "DisplayTask.h"

class DisplayTask_Flash : public DisplayTask {
public:
	bool refresh(uint16_t displayData[]) override;

	uint16_t symbols[6];
	unsigned long endTime;

protected:
	const bool _canShow() override;
};

#endif

