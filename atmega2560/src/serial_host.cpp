#include <Arduino.h>
#include <EEPROM.h>
#include <avr/wdt.h>
#include <MemoryUsage.h>

#include "serial_host.h"
#include "serial_wifi.h"
#include "const.h"
#include "version.h"
#include "reset.h"
#include "variables.h"
#include "rtc.h"

#include "Display.h"

HostSerial::HostSerial(HardwareSerial& serial) : CommandSerial(serial) {

}

void HostSerial::handle() {
    byte tmpData;

    switch (this->command) {
        // T HH II SS DD MM YY
        // H = Hours, I = Minutes, S = Seconds, D = Day of month, M = month, Y = year (ALL Dec)
        // Sets the time on the clock
        // ^T175630010318|17199
    case 'T':
        if (this->buffer.length() < 12) {
            this->reply(F("BAD (Invalid length; expected 12)"));
            break;
        }

        tmElements_t tm;
        tm.Hour = this->buffer.substring(0, 2).toInt();
        tm.Minute = this->buffer.substring(2, 4).toInt();
        tm.Second = this->buffer.substring(4, 6).toInt();
        tm.Day = this->buffer.substring(5, 8).toInt();
        tm.Month = this->buffer.substring(8, 10).toInt();
        tm.Year = y2kYearToTm(this->buffer.substring(10, 12).toInt());
        rtcSetTime(tm);

        hostSerial.echoFirst(F("Time changed: "));
        hostSerial.sendEnd(this->buffer);

        this->reply(F("OK"));

        break;
        // H
        // Pings the display ("Hello")
        // ^H|10300
    case 'H':
        this->reply(F("OK " FW_VERSION));
        break;
        // X
        // Performs a display reset of all modes
        // ^X|14861
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
        // ^P04|-7920
        // ^P01|-20043
        // ^P|-17659
    case 'P':
        if (this->buffer.length() < 2) {
            displayAntiPoisonOff();
        }
        else {
            displayAntiPoison(this->buffer.substring(0, 2).toInt());
        }
        this->reply(F("OK"));
        break;
        // F [MMMMMMMM DDD NNNNNN [RR GG BB]]
        // M = milliseconds (Dec), D = dots (Bitmask Dec) to show the message, N = Nixie message (Dec), R = Red (Hex), G = Green (Hex), B = Blue (Hex)
        // Shows a "flash"/"alert" message on the clock (will show this message instead of the time for <M> milliseconds. Does not use/reset hold when 0). Dots are bit 1 for lower and bit 2 for upper. Turned off when HIGH
        // If sent without any parameters, resets current flash message and goes back to clock mode
        // ^F000010002131337 *012|26595
    case 'F':
        if (this->buffer.length() < 20) {
            if (this->buffer.length() < 1) {
                DisplayTask::cycleDisplayUpdater();
                this->reply(F("OK"));
                break;
            }
            this->reply(F("BAD (Invalid length; expected 20 or 0)"));
            break;
        }

        displayFlash.setDataFromSerial(this->buffer);

        this->reply(F("OK"));
        break;
        // C [MMMMMMMM [RR GG BB]]
        // M = Time in ms (Dec), R = Red (Hex), G = Green (Hex), B = Blue (Hex)
        // Starts a countdown for <M> ms. Stops countdown if <M> = 0
        // ^C00010000|9735
        // ^C|-26281
    case 'C':
        if (this->buffer.length() < 8) {
            displayCountdown.reset();
        }
        else {
            displayCountdown.timeReset = this->buffer.substring(0, 8).toInt();
            displayCountdown.start();
        }
        displayCountdown.setColorFromInput(8, EEPROM_STORAGE_COUNTDOWN_RGB, this->buffer);
        displayCountdown.showIfPossibleOtherwiseRotateIfCurrent();
        this->reply(F("OK"));
        break;
        // W C [RR GG BB]
        // C = subcommand, R = Red (Hex), G = Green (Hex), B = Blue (Hex)
        // Controls the stopwatch. R for reset/disable, P for pause, U for un-pause, S for start/restart
        // ^WS|-8015
        // ^WR|-3952
    case 'W':
        if (this->buffer.length() < 1) {
            this->reply(F("BAD (Invalid length; expected 1)"));
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
        default:
            tmpData = false;
            this->reply(F("BAD (Invalid C)"));
            break;
        }
        if (tmpData) {
            displayStopwatch.showIfPossibleOtherwiseRotateIfCurrent();
            this->reply(F("OK"));
        }
        break;
        // ^E0|-9883
        // ^E1|-14012
        // ^E2|-1753
    case 'E':
        if (this->buffer.length() < 1) {
            this->reply(F("BAD (Invalid length; expected 1)"));
            break;
        }
        currentEffect = (DisplayEffect)(this->buffer[0] - '0');
        this->reply(F("OK"));
        break;
        // ^D|-5712
        // ^D111|-15634
    case 'D':
        this->replyFirst(F("OK "));
        this->sendEnd(String(mu_freeRam()));
        break;
    case 'L':
        if (this->buffer.length() < 1) {
            this->reply(F("BAD (Invalid length; expected 1)"));
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
            this->reply(F("BAD (Invalid argument)"));
            break;
        }
        break;
    case 'N':
        if (this->buffer.length() < 1) {
            this->reply(F("BAD (Invalid length; expected 1)"));
            break;
        }
        wifiSerial.send(this->buffer);
        break;
    case '<': // Echo, ignore
        break;
    case '$': // Command reply, ignore
        break;
    default:
        this->reply(F("BAD (Invalid command)"));
        break;
    }
}
