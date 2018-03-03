#ifndef _CRCSERIAL_H_INCLUDED
#define _CRCSERIAL_H_INCLUDED

String inputString;

void serialInit();
void serialSend(const String data);
bool serialReadNext();

#endif // _CRCSERIAL_H_INCLUDED

