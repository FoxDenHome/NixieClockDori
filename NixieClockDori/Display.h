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

const byte MASK_UPPER_DOTS = 1;
const byte MASK_LOWER_DOTS = 2;
const byte MASK_BOTH_DOTS = MASK_UPPER_DOTS | MASK_LOWER_DOTS;

#ifdef EFFECT_ENABLED
extern byte dataIsTransitioning[6];
extern uint16_t dataToDisplayOld[6];
#endif
extern uint16_t dataToDisplay[6];
extern byte dotMask;

#define setDotsConst setDots

extern unsigned long antiPoisonEnd;

void displayInit();

void displayAntiPoison(const unsigned long count);

byte makeDotMask(const bool upper, const bool lower);
void setDots(const bool upper, const bool lower);

uint16_t getNumber(const byte idx);

void insert1(const byte offset, const byte data, const bool trimLeadingZero);
bool insert2(const byte offset, const byte data, const bool trimLeadingZero);
bool showShortTime(const unsigned long timeMs, bool trimLZ);

#endif

