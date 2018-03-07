#pragma once

#define DISPLAY_NEEDS_TIMER1

void displayDriverInit();
void displayDriverLoop(const unsigned long curMicros);
