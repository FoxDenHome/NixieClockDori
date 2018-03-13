#pragma once

#include "DisplayTask.h"

class DisplayTask_Flash : public DisplayTask {
public:
	DisplayTask_Flash();

	bool refresh() override;

	byte symbols[3];
	unsigned long endTime = 0;

protected:
	const bool _canShow() override;
};
