#pragma once

#include <Arduino.h>

#include "serial.h"

class HostSerial : public CommandSerial {
public:
    HostSerial(HardwareSerial& _serial);
    
    int type() override;

protected:
    void handle() override;
};
