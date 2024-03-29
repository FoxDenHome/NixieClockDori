#pragma once

#define hexCharToNum(c) ((c <= '9') ? c - '0' : c - '7')
inline byte hexInputToByte(const byte offset, const String& data) {
	const byte msn = data[offset];
	const byte lsn = data[offset + 1];
	return (hexCharToNum(msn) << 4) + hexCharToNum(lsn);
}
