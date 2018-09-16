#pragma once

#include <Arduino.h>

extern String inputString;

void serialInit();

void serialSend(const String& str);
void serialSendFirst(const String& str);
void serialSendNext(const String& str);
void serialSendEnd();

template<typename T>
inline void serialSendN_(const T& str) {
	serialSendNext(str);
}

inline void serialSendN_() {

}

template<typename T, typename... Args>
inline void serialSendN_(const T& first, Args... args) {
	serialSendNext(first);
	serialSendN_(args...);
}

template<typename T, typename... Args>
inline void serialSendN(const T& first, Args... args) {
	serialSendFirst(first);
	serialSendN_(args...);
	serialSendEnd();
}

template<typename T>
inline void serialSendN(const T& first) {
	serialSend(first);
}

#define serialSendF(a) { serialSend(F(a)); }

bool serialReadNext();
