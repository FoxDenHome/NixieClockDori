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

	# isoweekday returns 1 for Monday until 7 for Sunday
	# We need 1 for Sunday and 2 for Monday (and 7 for Saturday)
	w = now.isoweekday() + 1
	if w == 8:
		w = 1
	return sendCommand("T%02d%02d%02d%02d%02d%02d%01d" % (h, m, s, d, mon, y, w))

if __name__ == "__main__":
	print("Connecting...")
	print(connect(argv[1]))
	while True:
		print("Syncing time...")
		print(syncTime())
		sleep(300)
