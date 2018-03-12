#pragma once

#include <Arduino.h>

#include "config.h"

enum DisplayEffect {
	NONE = 0,
	TRANSITION,
	SLOT_MACHINE,
	FIRST_INVALID,
};
extern DisplayEffect currentEffect;

const uint16_t ALL_TUBES = (1 << 10) - 1; // Bitmask to enable all tubes
const uint16_t NO_TUBES = 0;

extern byte dotMask;
extern bool doFlip;
extern bool renderAlways;
extern uint16_t *displayDataFront;
extern uint16_t *displayDataBack;
extern unsigned long dataIsTransitioning[6];
extern uint16_t dataToDisplayPrevious[6];

void displayInit();
void displayLoop(const unsigned long curMicros);

void displayAntiPoisonOff();
void displayAntiPoison(const unsigned long count);

byte makeDotMask(const bool upper, const bool lower);

uint16_t getNumber(const byte idx);

void insert1(const byte offset, const byte data, const bool trimLeadingZero, uint16_t dataToDisplay[]);
bool insert2(const byte offset, const byte data, const bool trimLeadingZero, uint16_t dataToDisplay[]);
bool showShortTime(const unsigned long timeMs, bool trimLZ, uint16_t dataToDisplay[], bool alwaysLong);
