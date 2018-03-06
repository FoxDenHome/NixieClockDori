/********************/
/* PIN DEFINITIONS  */
/********************/
// Passes data from SPI chip to display while HIGH (pulled LOW during SPI write)
#define PIN_DISPLAY_LATCH 10
#if PIN_DISPLAY_LATCH > 7
#define PORT_DISPLAY_LATCH PORTB
#define PORT_MASK_DISPLAY_LATCH (1 << (PIN_DISPLAY_LATCH - 8))
#else
#define PORT_DISPLAY_LATCH PORTD
#define PORT_MASK_DISPLAY_LATCH (1 << PIN_DISPLAY_LATCH)
#endif

const byte PIN_HIZ = 8;  // Z state in registers outputs (while LOW level) Always LOW? */
const byte PIN_HIGH_VOLTAGE_ENABLE = 5;  // High Voltage (tube power) on while HIGH
const byte PIN_BUZZER = 2;  // Piezo buzzer pin

const byte PIN_LED_RED = 9;  // PWM/analog pin for all red LEDs
const byte PIN_LED_GREEN = 6;  // PWM/analog pin for all green LEDs
const byte PIN_LED_BLUE = 3;  // PWM/analog pin for all blue LEDs

#define PIN_BUTTON_SET A0
#define PIN_BUTTON_UP A2
#define PIN_BUTTON_DOWN A1

#define EEPROM_STORAGE_BASE 0
#define EEPROM_STORAGE_COUNTDOWN EEPROM_STORAGE_BASE
#define EEPROM_STORAGE_COUNTDOWN_RGB (EEPROM_STORAGE_COUNTDOWN + sizeof(unsigned long))
#define EEPROM_STORAGE_STOPWATCH_RGB (EEPROM_STORAGE_COUNTDOWN_RGB + 3)
#define EEPROM_STORAGE_CLOCK_RGB (EEPROM_STORAGE_STOPWATCH_RGB + 3)
#define EEPROM_STORAGE_DATE_AUTO (EEPROM_STORAGE_CLOCK_RGB + 3)
#define EEPROM_END (EEPROM_STORAGE_DATE_AUTO + sizeof(bool))

/*******************/
/* OTHER CONSTANTS */
/*******************/
const unsigned long ONE_SECOND_IN_MS = 1000UL;
const unsigned long ONE_MINUTE_IN_MS = ONE_SECOND_IN_MS * 60UL;
const unsigned long ONE_HOUR_IN_MS = ONE_MINUTE_IN_MS * 60UL;

