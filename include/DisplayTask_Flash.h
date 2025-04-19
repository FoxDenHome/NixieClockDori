#pragma once

#include <Arduino.h>

#include "DisplayTask.h"

class DisplayTask_Flash : public DisplayTask {
public:
	DisplayTask_Flash();

	bool refresh() override;

	unsigned long duration = 0;
	unsigned long lastUpdate = 0;
	bool allowEffects = true;

	void setDataFromSerial(const String& data);

protected:
	bool _isActive() const override;
};
