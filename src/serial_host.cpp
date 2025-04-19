#include <Arduino.h>
#include <EEPROM.h>
#include <avr/wdt.h>

#include "serial_host.h"
#include "serial_wifi.h"
#include "const.h"
#include "version.h"
#include "reset.h"
#include "variables.h"
#include "rtc.h"
#include "temperature.h"

#include "Display.h"

HostSerial::HostSerial(HardwareSerial& serial) : CommandSerial(serial) {

}

void HostSerial::handle() {
    byte tmpData;

    switch (this->command) {
        // T HH II SS DD MM YYYY
        // H = Hours, I = Minutes, S = Seconds, D = Day of month, M = month, Y = year (ALL Dec)
        // Sets the time on the clock
        // ^T175630010318
    case 'T':
        if (this->buffer.length() < 14) {
            this->reply(F("BAD Invalid length; expected 14"));
            break;
        }

        tmElements_t tm;
        tm.Hour = this->buffer.substring(0, 2).toInt();
        tm.Minute = this->buffer.substring(2, 4).toInt();
        tm.Second = this->buffer.substring(4, 6).toInt();
        tm.Day = this->buffer.substring(6, 8).toInt();
        tm.Month = this->buffer.substring(8, 10).toInt();
        tm.Year = CalendarYrToTm(this->buffer.substring(10, 14).toInt());
        rtcSetTime(tm);

        hostSerial.echoFirst(F("Time changed: "));
        hostSerial.sendEnd(this->buffer);

        this->reply(F("OK"));

        break;
        // S [D......]
        // Set temperature in 1/1000 deg C
        // ^S23000
    case 'S':
        if (this->buffer.length() < 1) {
            this->reply(F("BAD Invalid length; expected >= 1"));
            break;
        }
        temperatureSet(this->buffer.toFloat());
        this->reply(F("OK"));

        break;
        // ? [X]
        // Select a screen
        //     T = Time
        //     D = Date
        //     W = Stopwatch
        //     C = Countdown
        //     S = Temperature
        //     _ = current user-selection
        // ^?T
    case '?':
        if (this->buffer.length() < 1) {
            if (displayClock.isSelected()) {
                this->reply(F("OK T"));
                break;
            } else if (displayDate.isSelected()) {
                this->reply(F("OK D"));
                break;
            } else if (displayStopwatch.isSelected()) {
                this->reply(F("OK W"));
                break;
            } else if (displayCountdown.isSelected()) {
                this->reply(F("OK C"));
                break;
            } else if (displayTemp.isSelected()) {
                this->reply(F("OK S"));
                break;
            } else {
                this->reply(F("BAD Invalid screen"));
                break;
            }
            break;
        }

        tmpData = true;
        switch (this->buffer[0]) {
        case 'T':
            displayClock.select();
            break;
        case 'D':
            displayDate.select();
            break;
        case 'W':
            displayStopwatch.select();
            break;
        case 'C':
            displayCountdown.select();
            break;
        case 'S':
            displayTemp.select();
            break;
        case '_':
            break;
        default:
            tmpData = false;
            this->reply(F("BAD Invalid screen"));
            break;
        }

        if (tmpData) { 
            DisplayTask::current = DisplayTask::selected;
            DisplayTask::current->isDirty = true;
            this->reply(F("OK"));
        }

        break;
        // H
        // Pings the display ("Hello")
        // ^H
    case 'H':
        this->reply(F("OK " FW_VERSION));
        break;
        // X
        // Performs a display reset of all modes
        // ^X
    case 'X':
        MCUSR = 0;
        wdt_disable();

        for (uint16_t i = 0; i < EEPROM.length(); i++) {
            EEPROM.write(i, 0);
        }
        this->reply(F("OK"));

        forceReset();
        break;
        // P [CC]
        // C = Count (Dec)
        // Performs an anti poisoning routine <C> times
        // ^P04
        // ^P01
        // ^P
    case 'P':
        if (this->buffer.length() < 2) {
            displayAntiPoisonOff();
        }
        else {
            displayAntiPoison(this->buffer.substring(0, 2).toInt());
        }
        this->reply(F("OK"));
        break;
        // F [MMMMMMMM DDD NNNNNNNNN [RR GG BB]]
        // M = milliseconds (Dec), D = dots (Bitmask Dec) to show the message, N = Nixie message (Str), R = Red (Hex), G = Green (Hex), B = Blue (Hex)
        // Shows a "flash"/"alert" message on the clock (will show this message instead of the time for <M> milliseconds. Does not use/reset hold when 0). Dots are bit 1 for lower and bit 2 for upper. Turned off when HIGH
        // If sent without any parameters, resets current flash message and goes back to clock mode
        // ^F000010002131337 *012
    case 'F':
        if (this->buffer.length() < 20) {
            if (this->buffer.length() < 1) {
                displayFlash.duration = 0;
                this->reply(F("OK"));
                break;
            }
            this->reply(F("BAD Invalid length; expected 20 or 0"));
            break;
        }

        displayFlash.setDataFromSerial(this->buffer);

        this->reply(F("OK"));
        break;
        // C C [MMMMMMMM [RR GG BB]]
        // C = subcommand, M = Time in ms (Dec), R = Red (Hex), G = Green (Hex), B = Blue (Hex)
        // Controls the countdown. R for reset/disable, P for pause, U for un-pause, S for start/restart
        //                         C for get current value, G for get configured value, N for no-op (just set configuration)
        // ^CS00010000
        // ^CR
    case 'C':
        if (this->buffer.length() < 1) {
            this->reply(F("BAD Invalid length; expected 1"));
            break;
        }

        tmpData = true;
        switch (this->buffer[0]) {
            case 'C':
                tmpData = false;
                this->reply(String(displayCountdown.getTime()));
                break;
            case 'G':
                tmpData = false;
                this->reply(String(displayCountdown.getResetTime()));
                break;
        }

        if (tmpData) {
            if (this->buffer.length() >= 9) {
                displayCountdown.timeReset = this->buffer.substring(1, 9).toInt();
            }
            displayCountdown.setColorFromInput(8, EEPROM_STORAGE_COUNTDOWN_RGB, this->buffer);

            tmpData = true;
            switch (this->buffer[0]) {
                case 'R':
                    displayCountdown.reset();
                    break;
                case 'P':
                    displayCountdown.pause();
                    break;
                case 'U':
                    displayCountdown.resume();
                    break;
                case 'S':
                    displayCountdown.start();
                    break;
                case 'N':
                    break;
                default:
                    tmpData = false;
                    this->reply(F("BAD Invalid C"));
                    break;
            }

            if (tmpData) {
                displayCountdown.showIfActiveOtherwiseShowSelected();
                this->reply(F("OK"));
            }
        }
        break;
        // W C [RR GG BB]
        // C = subcommand, R = Red (Hex), G = Green (Hex), B = Blue (Hex)
        // Controls the stopwatch. R for reset/disable, P for pause, U for un-pause, S for start/restart, N for no-op
        // ^WS
        // ^WR
    case 'W':
        if (this->buffer.length() < 1) {
            this->reply(F("BAD Invalid length; expected 1"));
            break;
        }
        displayStopwatch.setColorFromInput(1, EEPROM_STORAGE_STOPWATCH_RGB, this->buffer);
        tmpData = true;
        switch (this->buffer[0]) {
        case 'R':
            displayStopwatch.reset();
            break;
        case 'P':
            displayStopwatch.pause();
            break;
        case 'U':
            displayStopwatch.resume();
            break;
        case 'S':
            displayStopwatch.start();
            break;
        case 'G':
            tmpData = false;
            this->reply(String(displayStopwatch.getTime()));
            break;
        case 'N':
            break;
        default:
            tmpData = false;
            this->reply(F("BAD Invalid C"));
            break;
        }
        if (tmpData) {
            displayStopwatch.showIfActiveOtherwiseShowSelected();
            this->reply(F("OK"));
        }
        break;
        // ^E0
        // ^E1
        // ^E2
    case 'E': {
        if (this->buffer.length() < 1) {
            this->reply(F("BAD Invalid length; expected 1"));
            break;
        }
        const DisplayEffect effect = (DisplayEffect)(this->buffer[0] - '0');
        currentEffect = effect;
		EEPROM.put(EEPROM_STORAGE_CURRENT_EFFECT, effect);
        this->reply(F("OK"));
        break;
    }
        // ^L0
        // ^L1
    case 'L':
        if (this->buffer.length() < 1) {
            this->reply(F("BAD Invalid length; expected 1"));
            break;
        }

        switch (this->buffer[0]) {
        case '0':
            DisplayTask::buttonLock = false;
            this->reply(F("OK 0"));
            break;
        case '1':
            DisplayTask::buttonLock = true;
            this->reply(F("OK 1"));
            break;
        default:
            this->reply(F("BAD Invalid argument"));
            break;
        }
        break;
        // ^N [data]
    case 'N':
        if (this->buffer.length() < 1) {
            this->reply(F("BAD Invalid length; expected 1"));
            break;
        }
        wifiSerial.send(this->buffer);
        break;
    case '<': // Echo, ignore
        break;
    case '$': // Command reply, ignore
        break;
    default:
        this->reply(F("BAD Invalid command"));
        break;
    }
}

int HostSerial::type() {
    return 1;
}
