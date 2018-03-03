#ifndef _DISPLAY_H_INCLUDED
#define _DISPLAY_H_INCLUDED

#include <arduino.h>

#include "config.h"
#ifdef EFFECT_SLOT_MACHINE
#define EFFECT_ENABLED
#endif

const uint16_t ALL_TUBES = (1 << 10) - 1; // Bitmask to enable all tubes
const uint16_t NO_TUBES = 0;
const uint16_t INVALID_TUBES = 10000;

extern uint16_t dataToDisplay[6];

#define setDotsConst setDots

void displayInit();

void displayAntiPoison(const unsigned long count);

byte makeDotMask(const bool upper, const bool lower);
void setDots(const bool upper, const bool lower);
void setDotsMask(const byte mask);

uint16_t getNumber(const byte idx);

void insert1(const byte offset, const byte data, const bool trimLeadingZero);
bool insert2(const byte offset, const byte data, const bool trimLeadingZero);
bool showShortTime(const unsigned long timeMs, bool trimLZ);

#endif

