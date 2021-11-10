#include <Arduino.h>

#include "serial.h"
#include "config.h"

#define STATE_LOOKING_FOR_START 0
#define STATE_LOOKING_FOR_COMMAND 1
#define STATE_LOOKING_FOR_END 2

CommandSerial::CommandSerial(HardwareSerial& serial) {
	this->buffer = "";
	this->serial = &serial;
	this->commandState = STATE_LOOKING_FOR_START;
	this->buffer.reserve(128);
}

void CommandSerial::init() {
	this->serial->begin(115200);
}

void CommandSerial::sendFirst() {
	this->serial->print('^');
}

void CommandSerial::sendFirst(const String& text) {
    this->sendFirst();
	this->serial->print(text);
}

void CommandSerial::sendNext(const String& text) {
	this->serial->print(text);
}

void CommandSerial::sendEnd() {
	this->serial->print('\n');
}

void CommandSerial::sendEnd(const String& text) {
	this->serial->print(text);
	this->sendEnd();
}

void CommandSerial::send(const String& text) {
	this->sendFirst();
	this->serial->print(text);
	this->sendEnd();
}

void CommandSerial::loop() {
	while (this->serial->available()) {
        char data = this->serial->read();

        if (data == '\r' || data == '\t') {
            continue;
        }

        if (data == '^') {
            this->commandState = STATE_LOOKING_FOR_COMMAND;
            this->buffer = "";
            continue;
        }

        switch (this->commandState) {
            case STATE_LOOKING_FOR_START:
                break;
            case STATE_LOOKING_FOR_COMMAND:
                this->command = data;
                this->commandState = STATE_LOOKING_FOR_END;
                break;
            case STATE_LOOKING_FOR_END:
                if (data == '\n') {
                    this->handle();
                    this->commandState = STATE_LOOKING_FOR_START;
                    break;
                }
                this->buffer += data;
                if (this->buffer.length() > 100) {
                    this->commandState = STATE_LOOKING_FOR_START;
                    break;
                }
                break;
        }
    }
}
