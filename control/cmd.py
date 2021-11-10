#!/usr/bin/env python3

from cmdSerial import sendCommand, connect
from sys import argv

if __name__ == "__main__":
	print("Connecting...")
	print(connect(argv[1]))
	for arg in argv[2:]:
		print(sendCommand(arg))
