#!/bin/sh

esptool.py --port "$1" write_flash \
    --flash_size detect \
    0x00000 ESP8266_NonOS_AT_Bin_V1.7.4/bin/boot_v1.7.bin \
    0x01000 ESP8266_NonOS_AT_Bin_V1.7.4/bin/at/1024+1024/user1.2048.new.5.bin \
    0x1fb000 ESP8266_NonOS_AT_Bin_V1.7.4/bin/blank.bin \
    0x1fc000 ESP8266_NonOS_AT_Bin_V1.7.4/bin/esp_init_data_default_v08.bin \
    0xfe000 ESP8266_NonOS_AT_Bin_V1.7.4/bin/blank.bin \
    0x1fe000 ESP8266_NonOS_AT_Bin_V1.7.4/bin/blank.bin

