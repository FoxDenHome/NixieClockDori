#ifndef _DISPLAYTASK_STOPWATCH_H_INCLUDED
#define _DISPLAYTASK_STOPWATCH_H_INCLUDED

#include "DisplayTask.h"

class DisplayTask_Stopwatch : public DisplayTask {
public:
	DisplayTask_Stopwatch();
	bool render() override;

	void pause();
	void resume();
	void reset();
	void start();

protected:
	const bool _canShow() override;

private:
	bool running;
	unsigned long time;
	unsigned long lastCall;
};

#endif
