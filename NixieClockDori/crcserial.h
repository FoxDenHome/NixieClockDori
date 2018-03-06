#pragma once

#include <arduino.h>

extern String inputString;

void serialInit();

void serialSend(const String& str);
void serialSendFirst(const String &str);
void serialSendNext(const String &str);
void serialSendEnd();

#define serialSend1(a) { serialSend(a); }
#define serialSend2(a, b) { serialSendFirst(a); serialSendNext(b); serialSendEnd(); }
#define serialSend3(a, b, c) { serialSendFirst(a); serialSendNext(b); serialSendNext(c); serialSendEnd(); }
#define serialSend4(a, b, c, d) { serialSendFirst(a); serialSendNext(b); serialSendNext(c); serialSendNext(d); serialSendEnd(); }
#define serialSend5(a, b, c, d, e) { serialSendFirst(a); serialSendNext(b); serialSendNext(c); serialSendNext(d); serialSendNext(e); serialSendEnd(); }
#define serialSend6(a, b, c, d, e, f) { serialSendFirst(a); serialSendNext(b); serialSendNext(c); serialSendNext(d); serialSendNext(e); serialSendNext(f); serialSendEnd(); }

#define serialSendF(a) { serialSend(F(a)); }

bool serialReadNext();
