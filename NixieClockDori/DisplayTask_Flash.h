#pragma once

#include "DisplayTask.h"

class DisplayTask_Flash : public DisplayTask {
public:
	DisplayTask_Flash();

	bool refresh() override;

	uint16_t symbols[9];
	unsigned long endTime = 0;
	unsigned long lastUpdate = 0;
	bool allowEffects = true;

protected:
	bool _canShow() const override;
};
