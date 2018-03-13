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

const byte INVALID_TUBES = 11;
const byte ALL_TUBES = 11;
const byte NO_TUBES = 10;

extern byte dotMask;
extern bool doFlip;
extern bool renderAlways;
extern byte displayData[6];
extern byte dataIsTransitioning[6];
extern byte dataToDisplayPrevious[6];

void displayInit();
void displayLoop();

void displayAntiPoisonOff();
void displayAntiPoison(const unsigned long count);

byte makeDotMask(const bool upper, const bool lower);

byte getNumber(const byte idx);

void insert1(const byte offset, const byte data, const bool trimLeadingZero, byte dataToDisplay[]);
bool insert2(const byte offset, const byte data, const bool trimLeadingZero, byte dataToDisplay[]);
bool showShortTime(const unsigned long timeMs, bool trimLZ, byte dataToDisplay[], bool alwaysLong);
