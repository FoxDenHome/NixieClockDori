#pragma once

#include "buildinfo.h"

//#define CLOCK_TRIM_HOURS
#define EFFECT_SPEED 30
#define ANTI_POISON_DELAY 200
#define DISPLAY_RENDER_STEP 200
#define DISPLAY_CYCLE_PERIOD 30000000

#ifndef FW_VERSION
#ifdef GIT_COMMIT
#define FW_ADD_QUOTES(s) #s
#define FW_VERSION FW_ADD_QUOTES(GIT_BRANCH) " " FW_ADD_QUOTES(GIT_COMMIT)
#else
#define FW_VERSION "unknown"
#endif
#endif
