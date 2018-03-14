#pragma once

#include "DisplayTask.h"

class DisplayTask_Temperature : public DisplayTask {
public:
	bool refresh() override;

protected:
	const bool _canShow() override;

private:
	unsigned long nextTempRefresh = 0;
	int temp = 0;
};
