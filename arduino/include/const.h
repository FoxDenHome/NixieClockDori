#pragma once

#include "Display.h"

/********************/
/* PIN DEFINITIONS  */
/********************/
// Passes data from SPI chip to display while HIGH (pulled LOW during SPI write)
#define PIN_DISPLAY_LATCH 10

// Piezo buzzer pin
#define PIN_BUZZER 2

#define PIN_LED_RED 9
#define PIN_LED_GREEN 6
#define PIN_LED_BLUE 3

#define PIN_BUTTON_SET A0
#define PIN_BUTTON_UP A2
#define PIN_BUTTON_DOWN A1

#define PIN_TEMPERATURE 7

// Unused pins

// Connected to SHDN_C via logic level shifter, both not connected to any component otherwise
#define PIN_SHDN 5
// Connected to socket LS1 pin 2, which also has 4.7k pulldown to GND
#define PIN_LS A3
// Connected to IR receiver LED
#define PIN_IR 4

//#define PIN_DCF77 PCINT20
//const int LIMIT_DCF77 = 300;

/*******************/
/* EEPROM LOCATORS */
/*******************/
#define EEPROM_SIZEOF_RGB (sizeof(byte) * 3)

#define EEPROM_STORAGE_BASE 0

#define EEPROM_STORAGE_COUNTDOWN EEPROM_STORAGE_BASE
#define EEPROM_STORAGE_COUNTDOWN_RGB (EEPROM_STORAGE_COUNTDOWN + sizeof(unsigned long))
#define EEPROM_STORAGE_STOPWATCH_RGB (EEPROM_STORAGE_COUNTDOWN_RGB + EEPROM_SIZEOF_RGB)
#define EEPROM_STORAGE_CLOCK_RGB (EEPROM_STORAGE_STOPWATCH_RGB + EEPROM_SIZEOF_RGB)
#define EEPROM_STORAGE_DATE_AUTO (EEPROM_STORAGE_CLOCK_RGB + EEPROM_SIZEOF_RGB)
#define EEPROM_STORAGE_DATE_RGB (EEPROM_STORAGE_DATE_AUTO + sizeof(bool))
#define EEPROM_STORAGE_CURRENT_EFFECT (EEPROM_STORAGE_DATE_RGB + EEPROM_SIZEOF_RGB)
#define EEPROM_STORAGE_TEMPERATURE_RGB (EEPROM_STORAGE_CURRENT_EFFECT + sizeof(DisplayEffect))

#define EEPROM_STORAGE_END (EEPROM_STORAGE_TEMPERATURE_RGB + EEPROM_SIZEOF_RGB)

/*******************/
/* OTHER CONSTANTS */
/*******************/
const unsigned long ONE_SECOND_IN_MS = 1000UL;
const unsigned long ONE_MINUTE_IN_MS = ONE_SECOND_IN_MS * 60UL;
const unsigned long ONE_HOUR_IN_MS = ONE_MINUTE_IN_MS * 60UL;

