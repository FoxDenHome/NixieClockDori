#pragma once

#include <Arduino.h>

void gpsInit();
void gpsLoop();
void gpsSendDebug();

extern bool gpsToSerial;
