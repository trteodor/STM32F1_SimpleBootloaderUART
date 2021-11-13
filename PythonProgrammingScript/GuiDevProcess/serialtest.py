import serial
import time

serialPort = serial.Serial(
    port="COM6", baudrate=115200, bytesize=8, timeout=2, stopbits=serial.STOPBITS_ONE
)
serialString = ""  # Used to hold data coming over UART

serialPort.write(b"Hi How are you \r\n")
time.sleep(0.1)

serialString = serialPort.readline()

print(serialString.decode("Ascii"))
