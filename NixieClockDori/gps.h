#pragma once

#include <Arduino.h>

void gpsInit();
void gpsLoop();

extern bool gpsToSerial;
