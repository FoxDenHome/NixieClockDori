#pragma once

#include "DisplayTask.h"

class DisplayTask_Flash : public DisplayTask {
public:
	DisplayTask_Flash();

	bool refresh() override;

	byte symbols[5];
	unsigned long endTime = 0;
	unsigned long lastUpdate = 0;
	bool allowEffects = true;

protected:
	bool _canShow() const override;
};
