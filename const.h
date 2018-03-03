/********************/
/* PIN DEFINITIONS  */
/********************/
const byte PIN_DISPLAY_LATCH       = 10; // Passes data from SPI chip to display while HIGH (pulled LOW during SPI write)
const byte PIN_HIZ                 = 8;  // Z state in registers outputs (while LOW level) Always LOW? */
const byte PIN_HIGH_VOLTAGE_ENABLE = 5;  // High Voltage (tube power) on while HIGH
const byte PIN_BUZZER              = 2;  // Piezo buzzer pin

const byte PIN_LED_RED             = 9;  // PWM/analog pin for all red LEDs
const byte PIN_LED_GREEN           = 6;  // PWM/analog pin for all green LEDs
const byte PIN_LED_BLUE            = 3;  // PWM/analog pin for all blue LEDs

const byte PIN_BUTTON_SET          = A0; // "set" button
const byte PIN_BUTTON_UP           = A2; // "up" button
const byte PIN_BUTTON_DOWN         = A1; // "down" button

/*******************/
/* OTHER CONSTANTS */
/*******************/
const unsigned long ONE_SECOND_IN_MS = 1000UL;
const unsigned long ONE_MINUTE_IN_MS = ONE_SECOND_IN_MS * 60UL;
const unsigned long ONE_HOUR_IN_MS = ONE_MINUTE_IN_MS * 60UL;

