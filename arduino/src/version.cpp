#include <Arduino.h>
#include "crcserial.h"
#include "version.h"

#ifndef FW_VERSION_STR
#ifdef GIT_COMMIT
#define FW_ADD_QUOTES_H(s) #s
#define FW_ADD_QUOTES(s) FW_ADD_QUOTES_H(s)
#define FW_VERSION_STR FW_ADD_QUOTES(GIT_BRANCH) " " FW_ADD_QUOTES(GIT_COMMIT)
#else
#define FW_VERSION_STR "unknown"
#endif
#endif

const char FW_VERSION[] PROGMEM = {FW_VERSION_STR};
