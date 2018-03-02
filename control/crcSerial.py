from binascii import crc_hqx

def sendCommand(cmd):
    checksum = crc_hqx(cmd, 0xffff)
    data = "^%s|%d\n" % (cmd, checksum)
    print(data)
