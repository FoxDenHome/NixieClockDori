#ifndef _CRCSERIAL_H_INCLUDED
#define _CRCSERIAL_H_INCLUDED

#include <arduino.h>

extern String inputString;

void serialInit();

void serialSendSimple(const String data);
void serialSendFirst(const String str);
void serialSendNext(const String str);
void serialSendEnd();

bool serialReadNext();

#endif // _CRCSERIAL_H_INCLUDED

