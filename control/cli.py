#!/usr/bin/env python3

from cmdSerial import sendCommand, connect
from sys import argv

if __name__ == "__main__":
	print("Connecting...")
	print(connect(argv[1]))
	while True:
		print(sendCommand(input("$ ")))
