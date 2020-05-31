import matplotlib.pyplot as plt
import paho.mqtt.client as paho
import numpy as np
import serial
import time

Fs = 20.0  # how many points
Ts = 1.0/Fs # sampling interval

# XBee setting
serdev = '/dev/ttyUSB0'
s = serial.Serial(serdev, 9600)

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

s.write("ATID 0x3\r\n".encode())
char = s.read(3)
print("Set PAN ID 0x3.")
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
char = s.read(3)
print("Exit AT mode.")
print(char.decode())

print("start sending RPC")
    # send RPC to remote
collected = []
publish_data = []
# get data
for i in range(11): # don't use first data
    # s.write(bytes("/getAcc/run\r", 'UTF-8'))
    s.write("/getAcc/run\r".encode())
    if (i >= 1):
        for j in range(2):
            line = s.readline()
            publish_data.append(line.decode(errors = 'ignore'))
            print(line.decode(errors = 'ignore'))
            tmp_L = line.split()
            collected.append(int(tmp_L[-1]))
    time.sleep(1)
# print(publish_data)
data_collect = []
# draw the result
for i in range(len(collected) - 1):
    if (collected[i + 1] < collected[i]):
        data_collect.append(collected[i])

# t = np.arange(0, len(collected), 1.0)
t = np.linspace(0, 20, len(data_collect))
plt.plot(t, data_collect)
plt.ylim(1, 15)
plt.xlabel('timestamp')
plt.ylabel('number')
plt.title('# of collected data plot')
plt.show()

# publish data
mqttc = paho.Client()

# Settings for connection
host = "localhost"
topic= "Mbed"
port = 1883

# Callbacks
def on_connect(self, mosq, obj, rc):
    print("Connected rc: " + str(rc))

def on_message(mosq, obj, msg):
    print("[Received] Topic: " + msg.topic + ", Message: " + str(msg.payload) + "\n")

def on_subscribe(mosq, obj, mid, granted_qos):
    print("Subscribed OK")

def on_unsubscribe(mosq, obj, mid, granted_qos):
    print("Unsubscribed OK")

# Set callbacks
mqttc.on_message = on_message
mqttc.on_connect = on_connect
mqttc.on_subscribe = on_subscribe
mqttc.on_unsubscribe = on_unsubscribe

# Connect and subscribe
print("Connecting to " + host + "/" + topic)
mqttc.connect(host, port=1883, keepalive=60)
mqttc.subscribe(topic, 0)

mesg = 'X'.join(publish_data) # X denotes the delimiter
# mesg = "Hello, world!"
mqttc.publish(topic, mesg)
print(mesg)
time.sleep(1)

s.close()