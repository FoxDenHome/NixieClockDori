#pragma once

#include <Arduino.h>

#include "serial_host.h"

class WifiSerial : public HostSerial {
public:
    WifiSerial(HardwareSerial& _serial);
    
    int type() override;

protected:
    void handle() override;
};
