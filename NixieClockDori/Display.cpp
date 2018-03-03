#include "Display.h"
#include "const.h"

#ifdef EFFECT_ENABLED
byte dataIsTransitioning[6] = {0, 0, 0, 0, 0, 0};
uint16_t dataToDisplayOld[6] = {INVALID_TUBES, INVALID_TUBES, INVALID_TUBES, INVALID_TUBES, INVALID_TUBES, INVALID_TUBES};
#endif
uint16_t dataToDisplay[6] = {0, 0, 0, 0, 0, 0}; // This will be displayed on tubes
byte dotMask;

unsigned long antiPoisonEnd = 0;

void displayAntiPoison(const unsigned long count) {
  antiPoisonEnd = millis() + (ANTI_POISON_DELAY * 10UL * count);
}

uint16_t getNumber(const byte idx) {
  return 1 << (idx % 10);
}

byte makeDotMask(const bool upper, const bool lower) {
  return (upper ? 0 : MASK_UPPER_DOTS) | (lower ? 0 : MASK_LOWER_DOTS);
}

void setDots(const bool upper, const bool lower) {
  dotMask = makeDotMask(upper, lower);
}

bool showShortTime(const unsigned long timeMs, bool trimLZ) {
  if (timeMs >= ONE_HOUR_IN_MS) { // Show H/M/S
    setDotsConst(true, false);
    trimLZ = insert2(0, (timeMs / ONE_HOUR_IN_MS) % 100, trimLZ);
    trimLZ = insert2(2, (timeMs / ONE_MINUTE_IN_MS) % 60, trimLZ);
    insert2(4, (timeMs / ONE_SECOND_IN_MS) % 60, trimLZ);
    return true;
  } else { // Show M/S/MS
    setDotsConst(false, true);
    trimLZ = insert2(0, (timeMs / ONE_MINUTE_IN_MS) % 60, trimLZ);
    trimLZ = insert2(2, (timeMs / ONE_SECOND_IN_MS) % 60, trimLZ);
    insert2(4, (timeMs / 10UL) % 100, trimLZ);
    return false; // Don't allow transition effects on rapid timer
  }
}

void insert1(const byte offset, const byte data, const bool trimLeadingZero) {
  if (data == 0 && trimLeadingZero) {
    dataToDisplay[offset] = 0;
  } else {
    dataToDisplay[offset] = getNumber(data);
  }
}

bool insert2(const byte offset, const byte data, const bool trimLeadingZero) {
  insert1(offset, data / 10, trimLeadingZero);
  insert1(offset + 1, data, trimLeadingZero);
  return data == 0;
}

