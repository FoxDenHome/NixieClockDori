from datetime import datetime
from crcSerial import sendCommand

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

    sendCommand("T%02d%02d%02d%02d%02d%02d%01d" % (h, m, s, d, mon, y, w))

if __name__ == "__main__":
    syncTime()
