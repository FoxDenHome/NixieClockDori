#pragma once

#include <Arduino.h>

#include "serial.h"

class ArduinoSerial : public CommandSerial {
public:
    ArduinoSerial(HardwareSerial& _serial);

protected:
    void handle() override;

private:
    void processEEPROMCommand(const int offset);
    void httpFlash();
    void replyFirst();
    void replyFirst(const String& reply);
    void reply(const String& reply);
};
