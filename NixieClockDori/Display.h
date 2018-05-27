#pragma once

#include <Arduino.h>

#include "config.h"

enum DisplayEffect {
	NONE = 0,
	TRANSITION,
	SLOT_MACHINE,
	FIRST_INVALID,
};
extern volatile DisplayEffect currentEffect;

const byte INVALID_TUBES = 0xF;
const byte ALL_TUBES = 11;
const byte NO_TUBES = 10;
#define makeBothTubes(MASK) (MASK | (MASK << 4))
const byte ALL_TUBES_BOTH = makeBothTubes(ALL_TUBES);
const byte NO_TUBES_BOTH = makeBothTubes(NO_TUBES);
const byte INVALID_TUBES_BOTH = makeBothTubes(INVALID_TUBES);

const byte MASK_UPPER_DOTS = 1;
const byte MASK_LOWER_DOTS = 2;
const byte MASK_BOTH_DOTS = MASK_UPPER_DOTS | MASK_LOWER_DOTS;

extern volatile byte dotMask;
extern volatile bool renderAlways;
extern volatile byte displayData[3];
extern volatile byte dataIsTransitioning[3];
extern volatile byte dataToDisplayPrevious[3];
extern volatile bool renderNoMultiplex;

void displayInit();
void displayLoop();

void displayAntiPoisonOff();
void displayAntiPoison(const unsigned long count);

inline byte getNumber(const byte idx) {
	return idx % 10;
}

inline byte getNumberBoth(const byte idx) {
	return getNumber(idx) | (getNumber(idx) << 4);
}

inline byte makeDotMask(const bool upper, const bool lower) {
	return (upper ? 0 : MASK_UPPER_DOTS) | (lower ? 0 : MASK_LOWER_DOTS);
}

void insert1(const byte offset, const byte data, const bool trimLeadingZero);
bool insert2(const byte offset, const byte data, const bool trimLeadingZero);
bool showShortTime(const unsigned long timeMs, bool trimLZ, bool alwaysLong);
