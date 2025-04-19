#pragma once

#include "config.h"

#ifdef ENABLE_ESPPROXY

#include "serial.h"

void initESPProxy(CommandSerial *serial, uint8_t init);
void loopESPProxy();
extern bool ESPPRoxyMode;

#endif
