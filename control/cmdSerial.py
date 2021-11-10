from serial import Serial

bytesHasTwoArgs = False
try:
	test = bytes("test", "ascii")
	bytesHasTwoArgs = True
except:
	pass

def _bytes(data):
	if bytesHasTwoArgs:
		return bytes(data, "ascii")
	return bytes(data)

nixieCOM = Serial()

DOUBLE_CHARS = set([ord('N')])

def _readline(data, retryAfter = 5):
	i = 0
	while True:
		i += 1

		if i % retryAfter == 0:
			nixieCOM.write(data)
			nixieCOM.flush()

		line = nixieCOM.readline().decode("ascii").strip()

		lineStart = line.find('^')
		if lineStart < 0:
			continue

		line = line[lineStart + 1:]

		lineChar = ord(line[0])
		matchChar = data[1]
		try:
			matchChar = ord(matchChar)
		except:
			pass

		if lineChar in DOUBLE_CHARS and lineChar == matchChar:
			lineChar = ord(line[1])
			matchChar = data[2]
			try:
				matchChar = ord(matchChar)
			except:
				pass

		if lineChar == matchChar:
			return line
		else:
			print("Got async data: %s" % line)

def sendCommand(cmd):
	data = "^%s\n" % cmd
	data = _bytes(data)
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
