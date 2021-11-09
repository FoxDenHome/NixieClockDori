#pragma once

#include <Arduino.h>

void wifiInit();
void wifiLoop();
void wifiSerialSend(const String& str);
