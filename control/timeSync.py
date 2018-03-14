from datetime import datetime
from crcSerial import sendCommand, connect
from sys import argv
from time import sleep

def syncTime():
	startNow = datetime.now()
	now = datetime.now()
	while now.second == startNow.second:
		sleep(0.01)
		now = datetime.now()

	h = now.hour
	m = now.minute
	s = now.second
	d = now.day
	mon = now.month
	y = now.year % 100

	return sendCommand("T%02d%02d%02d%02d%02d%02d" % (h, m, s, d, mon, y))

if __name__ == "__main__":
	print("Connecting...")
	print(connect(argv[1]))
	while True:
		print("Syncing time...")
		print(syncTime())
		sleep(300)
