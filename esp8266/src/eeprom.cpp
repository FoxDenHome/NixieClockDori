#include <Arduino.h>
#include <ESP_EEPROM.h>

#include "eeprom.h"

void eepromCommit() {
    EEPROM.commit();
}

void eepromInit() {
    EEPROM.begin(EEPROM_END);
}

void eepromWrite(const int offset, const String& data) {
    const int len = data.length();
    EEPROM.write(offset, len);
    for (int i = 0; i < len; i++) {
        EEPROM.write(offset + i + 1, data[i]);
    }
}

String eepromRead(const int offset) {
    const int len = EEPROM.read(offset);
    if (len >= EEPROM_LEN_STRING) {
        return "";
    }
    char str[len + 1];
    for (int i = 0; i < len; i++) {
        str[i] = EEPROM.read(offset + i + 1);
    }
    str[len] = 0;
    return String(str);
}
