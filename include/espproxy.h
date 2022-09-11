#pragma once

#include "serial.h"

void initESPProxy(CommandSerial *serial, uint8_t init);
void loopESPProxy();
extern bool ESPPRoxyMode;
