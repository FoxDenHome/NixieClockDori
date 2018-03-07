#pragma once

#define DISPLAY_NEEDS_TIMER1

#ifdef TIMER1_C_PIN
#define autoAnalogWrite(PIN, VALUE) \
	if ((PIN) == TIMER1_A_PIN || (PIN) == TIMER1_B_PIN || (PIN) == TIMER1_C_PIN) { \
		Timer1.pwm(PIN, (VALUE) << 2); \
	} \
	else { \
		analogWrite(PIN, VALUE); \
	}
#else
#define autoAnalogWrite(PIN, VALUE) \
	if ((PIN) == TIMER1_A_PIN || (PIN) == TIMER1_B_PIN) { \
		Timer1.pwm(PIN, (VALUE) << 2); \
	} \
	else { \
		analogWrite(PIN, VALUE); \
	}
#endif

void displayDriverInit();
void displayDriverLoop(const unsigned long curMicros);
