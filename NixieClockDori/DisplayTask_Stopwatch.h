#ifndef _DISPLAYTASK_STOPWATCH_H_INCLUDED
#define _DISPLAYTASK_STOPWATCH_H_INCLUDED

#include "DisplayTask.h"

class DisplayTask_Stopwatch : public DisplayTask {
public:
	bool render(const Task* renderTask) override;

	void pause();
	void resume();
	void reset();
	void start();

protected:
	const bool _canShow() override;

private:
	bool running;
	unsigned long time;
};

#endif
