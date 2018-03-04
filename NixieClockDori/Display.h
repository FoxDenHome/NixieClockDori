#ifndef _DISPLAY_H_INCLUDED
#define _DISPLAY_H_INCLUDED

#include <arduino.h>

#include "config.h"
#ifdef EFFECT_SLOT_MACHINE
#define EFFECT_ENABLED
#endif

const uint16_t ALL_TUBES = (1 << 10) - 1; // Bitmask to enable all tubes
const uint16_t NO_TUBES = 0;

void displayInit();
void displayLoop(const unsigned long curMicros);

void displayAntiPoison(const unsigned long count);

byte makeDotMask(const bool upper, const bool lower);

uint16_t getNumber(const byte idx);

void insert1(const byte offset, const byte data, const bool trimLeadingZero, uint16_t dataToDisplay[]);
bool insert2(const byte offset, const byte data, const bool trimLeadingZero, uint16_t dataToDisplay[]);
bool showShortTime(const unsigned long timeMs, bool trimLZ, uint16_t dataToDisplay[], byte *dotMask);

#endif

