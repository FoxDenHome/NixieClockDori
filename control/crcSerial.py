from binascii import crc_hqx
from serial import Serial
#from io import TextIOWrapper, BufferedRWPair

nixieCOM = Serial()
nixieCOM.baudrate = 115200
nixieCOM.timeout = 1
#nixieIO = TextIOWrapper(BufferedRWPair(nixieCOM, nixieCOM))

def _crc(data):
	return crc_hqx(bytes(data, "ascii"), 0xffff)

def _readline(data, retryAfter = 5):
	i = 0
	while True:
		i += 1

		if i % retryAfter == 0:
			nixieCOM.write(data)
			nixieCOM.flush()

		try:
			line = nixieCOM.readline().decode("ascii").strip()
		except:
			continue

		lineStart = line.find('^')
		if lineStart < 0:
			continue

		line = line[lineStart + 1:]
		lineStart = line.find('|')
		if lineStart < 0:
			continue

		readCrc = int(line[lineStart+1:], 10)
		line = line[:lineStart]
		calcCrc = _crc(line)

		if readCrc == calcCrc:
			if ord(line[0]) == data[1]:
				return line
			else:
				print("Got async data: %s" % line)
		else:
			print("CRC mismatch: %s" % line)

def sendCommand(cmd):
	checksum = _crc(cmd)
	data = "^%s|%d\n" % (cmd, checksum)
	data = bytes(data, "ascii")
	nixieCOM.write(data)
	nixieCOM.flush()
	return _readline(data)

def connect(port):
	if nixieCOM.is_open:
		nixieCOM.close()
	nixieCOM.port = port
	nixieCOM.open()
	return sendCommand("H")
