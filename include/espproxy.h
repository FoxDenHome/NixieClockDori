#pragma once

#include "serial.h"

void initSerialProxy(CommandSerial *serial, uint8_t init);
void loopSerialProxy();
extern bool serialProxyMode;