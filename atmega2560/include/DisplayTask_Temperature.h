#pragma once

#include "DisplayTask.h"

class DisplayTask_Temperature : public DisplayTask {
public:
	DisplayTask_Temperature();

	bool refresh() override;

protected:
	bool _canShow() const override;

private:
	unsigned long lastTempRefresh = 0;
	int temp = 0;
};
