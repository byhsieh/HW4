import matplotlib.pyplot as plt
import numpy as np
import paho.mqtt.client as paho
import time

Fs = 20.0  # how many points
Ts = 1.0/Fs # sampling interval
t = np.arange(0, 10, 10 * Ts) # time vector; create Fs samples between 0 and 1.0 sec.
# y = np.arange(0, 1, Ts) # signal vector; create Fs samples
x_value = np.arange(0, 1, Ts)
y_value = np.arange(0, 1, Ts)
z_value = np.arange(0, 1, Ts)
log_value = np.arange(0, 1, Ts)
mqttc = paho.Client()

# Settings for connection
host = "localhost"
topic= "Mbed"
port = 1883

# Callbacks
def on_connect(self, mosq, obj, rc):
    print("Connected rc: " + str(rc))

def on_message(mosq, obj, msg):
    # print("[Received] Topic: " + msg.topic + ", Message: " + str(msg.payload) + "\n")
    # deal with the received data
    data = str(msg.payload).split('X')
    data[0] = data[0][4:]
    i = 0
    for item in data:
        raw_data = item.split()
        print(raw_data)
        x_value[i] = float(raw_data[0])
        y_value[i] = float(raw_data[1])
        z_value[i] = float(raw_data[2])
        log_value[i] = int(raw_data[3])
        i += 1

    # plot the result
    fig, ax = plt.subplots(2, 1) # 2 * 1
    # ax[0].title('Acceleration Plot')
    l1, = ax[0].plot(t, x_value, 'b')
    l2, = ax[0].plot(t, y_value, 'r')
    l3, = ax[0].plot(t, z_value, 'g')

    ax[0].legend([l1, l2, l3], ['x-acc', 'y-acc', 'z-acc'], loc = 'lower left')

    ax[0].set_xlabel('Time')
    ax[0].set_ylabel('Acc Value')

    ax[1].stem(t, log_value, use_line_collection = True)
    plt.ylim(-0.1, 1.1) # set the y-limit from 0 to 1
    # ax[1].plot(frq,abs(Y),'r') # plotting the spectrum
    ax[1].set_xlabel('Time')
    ax[1].set_ylabel('Tilt')
    plt.show()

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

# while(1):
#     mesg = "Hello, world!"
#     mqttc.publish(topic, mesg)
#     print(mesg)
#     time.sleep(1)
mqttc.loop_forever()