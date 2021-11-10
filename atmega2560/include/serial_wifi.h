#pragma once

#include <Arduino.h>

#include "serial.h"

class WifiSerial : public CommandSerial {
public:
    WifiSerial(HardwareSerial& _serial);

protected:
    void handle() override;
};
