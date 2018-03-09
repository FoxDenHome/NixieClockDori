from datetime import datetime
from crcSerial import sendCommand, connect
from sys import argv
from time import sleep

def syncTime():
	now = datetime.now()

	h = now.hour
	m = now.minute
	s = now.second
	d = now.day
	mon = now.month
	y = now.year % 100
	w = now.isoweekday()
	if w == 7:
	    w = 0
	return sendCommand("T%02d%02d%02d%02d%02d%02d%01d" % (h, m, s, d, mon, y, w))

if __name__ == "__main__":
	print("Connecting...")
	print(connect(argv[1]))
	while True:
		print("Syncing time...")
		print(syncTime())
		sleep(300)
