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

const byte INVALID_TUBES = 11;
const byte ALL_TUBES = 11;
const byte NO_TUBES = 10;

extern volatile byte dotMask;
extern volatile bool renderAlways;
extern volatile byte displayData[3];
extern volatile byte dataIsTransitioning[3];
extern volatile byte dataToDisplayPrevious[3];

void displayInit();
void displayLoop();

void displayAntiPoisonOff();
void displayAntiPoison(const unsigned long count);

byte makeDotMask(const bool upper, const bool lower);

byte getNumber(const byte idx);

void insert1(const byte offset, const byte data, const bool trimLeadingZero);
bool insert2(const byte offset, const byte data, const bool trimLeadingZero);
bool showShortTime(const unsigned long timeMs, bool trimLZ, bool alwaysLong);
