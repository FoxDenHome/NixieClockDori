#pragma once

#include <Arduino.h>

void mqttInit();
void mqttSend(const String& data);
