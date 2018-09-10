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

const uint16_t INVALID_TUBES = 0xFFFF;
const uint16_t ALL_TUBES = ((1 << 10) - 1);
const uint16_t NO_TUBES = 0;

extern volatile byte dotMask;
extern volatile bool renderAlways;
extern volatile uint16_t displayData[9];
extern volatile byte dataIsTransitioning[9];
extern volatile uint16_t dataToDisplayPrevious[9];

void displayInit();
void displayLoop();

void displayAntiPoisonOff();
void displayAntiPoison(const unsigned long count);

inline uint16_t getNumber(const byte idx) {
	return 1 << (idx % 10);
}

#define _MKDOT(x) (1 << x)

#define DOT_1_UP _MKDOT(1)
#define DOT_1_DOWN _MKDOT(0)
#define DOT_2_UP _MKDOT(3)
#define DOT_2_DOWN _MKDOT(2)
#define DOT_3_UP _MKDOT(5)
#define DOT_3_DOWN _MKDOT(4)

#define _MKSYM(x) (1 << x)

#define SYMBOL_PERCENT _MKSYM(0)
#define SYMBOL_M _MKSYM(1)
#define SYMBOL_P _MKSYM(2)
#define SYMBOL_m _MKSYM(3)
#define SYMBOL_K _MKSYM(4)
#define SYMBOL_n _MKSYM(5)
#define SYMBOL_MICRO _MKSYM(6)
#define SYMBOL_DEGREES_C _MKSYM(7)

void insert1(const byte offset, const byte data, const bool trimLeadingZero);
bool insert2(const byte offset, const byte data, const bool trimLeadingZero);
bool showShortTime(const unsigned long timeMs, bool trimLZ, bool alwaysLong);
