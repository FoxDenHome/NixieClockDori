#pragma once

#include "DisplayTask.h"

class DisplayTask_Temperature : public DisplayTask {
public:
	bool refresh() override;

protected:
	bool _canShow() const override;

private:
	unsigned long nextTempRefresh = 0;
	int temp = 0;
};
