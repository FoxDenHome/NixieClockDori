#pragma once

#include <Arduino.h>

#include "serial_host.h"

class WifiSerial : public HostSerial {
public:
    WifiSerial(HardwareSerial& _serial);

protected:
    void handle() override;
};
