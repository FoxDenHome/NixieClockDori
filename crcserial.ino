#include <FastCRC.h>
#include "crcserial.h"

FastCRC16 crc;
String inputChecksum;

#define stringCRC(str) crc.ccitt((const uint8_t*)str.c_str(), str.length())
#define stringCRCUpdate(str) crc.ccitt_upd((const uint8_t*)str.c_str(), str.length())

void serialInit() {
  Serial.begin(2000000);
  inputChecksum.reserve(16);
  inputString.reserve(32);
}

void serialSend(const String data) {
  Serial.print('^');
  Serial.print(data);
  Serial.print('|');
  Serial.println(String(stringCRC(data)));
}

bool serialReadNext() {
  static bool receivedStart = false;
  static bool inChecksum = false;

  const char inChar = (char)Serial.read();
  if (inChar == '\r' || inChar == ' ') { // Ignore those always
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
      serialSend(F("< Double checksum delimiter. Buffer reset."));
      return false;
    }
    inChecksum = true;
    return false;
  }

  if (inChar == '\n') {
    receivedStart = false;
    if (!inChecksum) {
      serialSend("> NOCRC " + inputString);
      return false;
    }
    int computedCRC = stringCRC(inputString);
    int receivedCRC = inputChecksum.toInt();
    if (computedCRC != receivedCRC) {
      serialSend("> BADCRC " + String(computedCRC) + " " + String(receivedCRC) + " " + inputString);
      return false;
    }
    serialSend("> OK " + inputString);
    return true;
  }

  if (inChecksum) {
    inputChecksum += inChar;
  } else {
    inputString += inChar;
  }
  if (inputString.length() >= 30) {
    receivedStart = false;
    serialSend(F("< Serial line too long. Buffer reset."));
  }
  return false;
}

