#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#define hexCharToNum(c) ((c <= '9') ? c - '0' : c - '7')
inline byte hexInputToByte(const byte offset) {
	const byte msn = inputString[offset];
	const byte lsn = inputString[offset + 1];
	return (hexCharToNum(msn) << 4) + hexCharToNum(lsn);
}

#endif
