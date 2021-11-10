#pragma once

#include <Arduino.h>

class CommandSerial {
public:
	CommandSerial(HardwareSerial& _serial);

	void init();
	void loop();

	void sendFirst(const String& text);
	void sendNext(const String& text);
	void sendEnd(const String& text);
	void send(const String& text);

protected:
	virtual void handle() = 0;

	HardwareSerial* serial;
	String buffer;
	byte commandState;
	char command;
};
