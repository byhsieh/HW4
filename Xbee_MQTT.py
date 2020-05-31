import serial
import time
import matplotlib.pyplot as plt
import numpy as np

serdev0 = '/dev/ttyACM0'
s0 = serial.Serial(serdev0,9600)

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

ACC_X = []
ACC_Y = []
ACC_Z = []


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

for i in range(0,20):

    line=s0.readline() # Read an echo string from K66F terminated with '\n'

    num = list(map(float,line.split()))

    # print (num)

    ACC_X.append(num[0])
    ACC_Y.append(num[1])
    ACC_Z.append(num[2])

fig, ax = plt.subplots(2, 1)

ax[0].plot(t,num)
ax[0].set_xlabel('timestamp')
ax[0].set_ylabel('number')
ax[0].set_title('# collected data plot')

ax[1].plot(t, ACC_X, color="blue",  label="x-acc")
ax[1].plot(t, ACC_Y, color="red",   label="y-acc")
ax[1].plot(t, ACC_Z, color="green", label="z-acc")
ax[1].legend(loc='lower right')
ax[1].set_xlabel('timestamp')
ax[1].set_ylabel('acc value')
ax[1].set_title('Acceleration Plot')

plt.show()

s.close()