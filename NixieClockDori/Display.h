#pragma once

#include <Arduino.h>

#include "config.h"

enum DisplayEffect {
	NONE = 0,
	//TRANSITION,
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
extern volatile byte displayData[5];
extern volatile byte dataIsTransitioning[5];
extern volatile byte dataToDisplayPrevious[5];

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

#define _MKDOT(x) (1 << x)

#define DOT_1_UP _MKDOT(1)
#define DOT_1_DOWN _MKDOT(0)
#define DOT_2_UP _MKDOT(3)
#define DOT_2_DOWN _MKDOT(2)
#define DOT_3_UP _MKDOT(5)
#define DOT_3_DOWN _MKDOT(4)

#define _MKSYM(x) x

#define SYMBOL_PERCENT _MKSYM(0)
#define SYMBOL_M _MKSYM(1)
#define SYMBOL_MICRO2 _MKSYM(2)
#define SYMBOL_m _MKSYM(3)
#define SYMBOL_K _MKSYM(4)
#define SYMBOL_n _MKSYM(5)
#define SYMBOL_MICRO _MKSYM(6)
#define SYMBOL_DEGREES_C _MKSYM(7)

void insert1(const byte offset, const byte data, const bool trimLeadingZero);
bool insert2(const byte offset, const byte data, const bool trimLeadingZero);
bool showShortTime(const unsigned long timeMs, bool trimLZ, bool alwaysLong);
