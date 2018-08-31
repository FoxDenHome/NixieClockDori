from binascii import crc_hqx
from serial import Serial

nixieCOM = Serial()

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
		except KeyboardInterrupt:
			raise
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
			#else:
			#	print("Got async data: %s" % line)
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
	nixieCOM.dsrdtr = False
	nixieCOM.rtscts = False
	nixieCOM.rts = False
	nixieCOM.dtr = False
	nixieCOM.baudrate = 115200
	nixieCOM.timeout = 1
	nixieCOM.open()
	return sendCommand("H")
