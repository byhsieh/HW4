import serial
import time
import matplotlib.pyplot as plt
import numpy as np

# XBee setting
serdev = '/dev/ttyUSB0'
s = serial.Serial(serdev,9600,timeout=3)

s.write("+++".encode())
char = s.read(2)
print("Enter AT mode.")
print(char.decode())

s.write("ATMY 0x196\r\n".encode())
char = s.read(3)
print("Set MY 0x196.")
print(char.decode())

s.write("ATDL 0x296\r\n".encode())
char = s.read(3)
print("Set DL 0x296.")
print(char.decode())

s.write("ATID 0x2\r\n".encode())
char = s.read(3)
print("Set PAN ID 0x2.")
print(char.decode())

s.write("ATWR\r\n".encode())
char = s.read(3)
print("Write config.")
print(char.decode())

s.write("ATMY\r\n".encode())
char = s.read(4)
print("MY :")
print(char.decode())

s.write("ATDL\r\n".encode())
char = s.read(4)
print("DL : ")
print(char.decode())

s.write("ATCN\r\n".encode())
char = s.read(4)
print("Exit AT mode.")
print(char.decode())

print("start sending RPC")

t=np.arange(0,20,1)
num=np.arange(0,20,1)
xbeenum=[]
count=0

while True:
    s.write("/query/run\r".encode())
    line=s.read(2)
    print("read:")
    print(line.decode())
    xbeenum.append(line)
    count=count+1
    time.sleep(1)
    if count==21 :
        break

for i in range(0,19):
    num[i]=xbeenum[i+1]
    print(num[i])
num[19]=2

fig, ax = plt.subplots(111)

ax.plot(t,num)
ax.set_xlabel('timestamp')
ax.set_ylabel('number')
ax.set_title('# collected data plot')

plt.show()

s.close()