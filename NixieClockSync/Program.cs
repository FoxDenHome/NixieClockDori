using System;
using System.IO.Ports;
using System.Text;
using System.Threading;

namespace NixieClockSync
{
    class Program
    {
        static SerialPort serialPort;

        static void Main(string[] args)
        {
            serialPort = new SerialPort();

            serialPort.PortName = args[0];
            serialPort.BaudRate = 115200;
            serialPort.DataBits = 8;
            serialPort.Parity = Parity.None;
            serialPort.StopBits = StopBits.One;
            serialPort.Handshake = Handshake.None;

            serialPort.WriteTimeout = 5000;
            serialPort.ReadTimeout = 5000;

            serialPort.Open();

            Console.Out.WriteLine("[C] " + SendAndWaitReply("H"));
            Console.Out.WriteLine("[C] " + SendAndWaitReply("L1"));
            while (true)
            {
                SyncTime();
                Thread.Sleep(300 * 1000);
            }
        }

        // From: https://stackoverflow.com/a/34943214
        static string ComputeCrc(string str)
        {
            byte[] bytes = Encoding.ASCII.GetBytes(str);

            const ushort poly = 4129;
            ushort[] table = new ushort[256];
            ushort initialValue = 0xffff;
            ushort temp, a;
            ushort crc = initialValue;
            for (int i = 0; i < table.Length; ++i)
            {
                temp = 0;
                a = (ushort)(i << 8);
                for (int j = 0; j < 8; ++j)
                {
                    if (((temp ^ a) & 0x8000) != 0)
                        temp = (ushort)((temp << 1) ^ poly);
                    else
                        temp <<= 1;
                    a <<= 1;
                }
                table[i] = temp;
            }

            for (int i = 0; i < bytes.Length; ++i)
            {
                crc = (ushort)((crc << 8) ^ table[((crc >> 8) ^ (0xff & bytes[i]))]);
            }

            return crc.ToString();
        }

        static void SyncTime()
        {
            Console.Out.WriteLine("[A] Starting sync. Waiting for next full second to send sync pulse...");
            Thread.Sleep(1000 - DateTime.Now.Millisecond);
            Console.Out.WriteLine("[C] " + SendAndWaitReply("T" + DateTime.Now.ToString("HHmmssddMMyy")));
            Console.Out.WriteLine("[A] Sync done!");
        }

        static string SendAndWaitReply(string data, int retryAfter = 5)
        {
            string sendLine = "^" + data + "|" + ComputeCrc(data);

            int i = 0;
            while (true)
            {
                if (i % retryAfter == 0)
                {
                    serialPort.WriteLine(sendLine);
                }
                i++;

                string line;
                try
                {
                    line = serialPort.ReadLine();
                }
                catch (TimeoutException)
                {
                    continue;
                }

                int lineStart = line.IndexOf('^');
                if (lineStart < 0)
                {
                    continue;
                }

                string[] lineSplit = line.Substring(1).Trim().Split('|');
                if (lineSplit.Length != 2)
                {
                    continue;
                }

                string readData = lineSplit[0];
                string readCrc = lineSplit[1];
                string correctCrc = ComputeCrc(readData);

                if (readData.Length > 0 && readCrc == correctCrc && data[0] == readData[0])
                {
                    return readData;
                }
            }
        }
    }
}
