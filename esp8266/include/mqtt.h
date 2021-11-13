#pragma once

#include <Arduino.h>

void mqttInit();
void mqttLoop();
void mqttSend(const String& data);
