#include <FastCRC.h>
#include "crcserial.h"

FastCRC16 crc;
String inputString;
String inputChecksum;

#define stringCRC(str) crc.ccitt((const uint8_t*)str.c_str(), str.length())
#define stringCRCUpdate(str) crc.ccitt_upd((const uint8_t*)str.c_str(), str.length())

void serialSend(const String& str) {
	serialSendFirst(str);
	serialSendEnd();
}

void serialInit() {
	Serial.begin(115200);
	inputChecksum.reserve(16);
	inputString.reserve(32);
}

uint16_t crcCalc;
void serialSendFirst(const String &str) {
	Serial.print('^');
	Serial.print(str);
	crcCalc = stringCRC(str);
}
void serialSendNext(const String &str) {
	Serial.print(str);
	crcCalc = stringCRCUpdate(str);
}
void serialSendEnd() {
	Serial.print('|');
	Serial.println(crcCalc);
}
void serialSendSimple(const String &str) {
	serialSendFirst(str);
	serialSendEnd();
}

bool serialReadNext() {
	static bool receivedStart = false;
	static bool inChecksum = false;

	const char inChar = (char)Serial.read();
	if (inChar == '\r' || inChar == '\t') { // Ignore those always
		return false;
	}

	if (inChar == '^') {
		receivedStart = true;
		inChecksum = false;
		inputString = "";
		inputChecksum = "";
		return false;
	}

	if (!receivedStart) {
		return false;
	}

	if (inChar == '|') {
		if (inChecksum) {
			receivedStart = false;
			serialSendSimple(F("< Double checksum delimiter. Buffer reset."));
			return false;
		}
		inChecksum = true;
		return false;
	}

	if (inChar == '\n') {
		receivedStart = false;
		if (!inChecksum) {
			serialSendN(F("> NOCRC "), inputString);
			return false;
		}
		int computedCRC = stringCRC(inputString);
		int receivedCRC = inputChecksum.toInt();
		if (computedCRC != receivedCRC) {
			serialSendN(F("> BADCRC "), String(computedCRC), " ", String(receivedCRC), " ", inputString);
			return false;
		}
		serialSendN(F("> OK "), inputString);
		return true;
	}

	if (inChecksum) {
		inputChecksum += inChar;
	}
	else {
		inputString += inChar;
	}
	if (inputString.length() >= 30) {
		receivedStart = false;
		serialSendSimple(F("< Serial line too long. Buffer reset."));
	}
	return false;
}

