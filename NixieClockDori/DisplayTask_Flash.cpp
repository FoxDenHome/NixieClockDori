#include "DisplayTask_Flash.h"
#include "Display.h"

const bool DisplayTask_Flash::_canShow() {
  return this->endTime > 0 && millis() < this->endTime;
}

bool DisplayTask_Flash::render(const Task* renderTask) {
  for (byte idx = 0; idx < 6; idx++) {
    dataToDisplay[idx] = this->symbols[idx];
  }
  dotMask = this->dotsMask;

  return true;
}

