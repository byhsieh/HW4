#include "mbed.h"
#include "mbed_events.h"
#include "mbed_rpc.h"
#include "fsl_port.h"
#include "fsl_gpio.h"

#include "MQTTNetwork.h"
#include "MQTTmbed.h"
#include "MQTTClient.h"

#include "math.h"
#include "iostream"

using namespace std;

#define UINT14_MAX        16383

// FXOS8700CQ I2C address
#define FXOS8700CQ_SLAVE_ADDR0 (0x1E<<1) // with pins SA0=0, SA1=0
#define FXOS8700CQ_SLAVE_ADDR1 (0x1D<<1) // with pins SA0=1, SA1=0
#define FXOS8700CQ_SLAVE_ADDR2 (0x1C<<1) // with pins SA0=0, SA1=1
#define FXOS8700CQ_SLAVE_ADDR3 (0x1F<<1) // with pins SA0=1, SA1=1

// FXOS8700CQ internal register addresses
#define FXOS8700Q_STATUS 0x00
#define FXOS8700Q_OUT_X_MSB 0x01
#define FXOS8700Q_OUT_Y_MSB 0x03
#define FXOS8700Q_OUT_Z_MSB 0x05
#define FXOS8700Q_M_OUT_X_MSB 0x33
#define FXOS8700Q_M_OUT_Y_MSB 0x35
#define FXOS8700Q_M_OUT_Z_MSB 0x37
#define FXOS8700Q_WHOAMI 0x0D
#define FXOS8700Q_XYZ_DATA_CFG 0x0E 
#define FXOS8700Q_CTRL_REG1 0x2A
#define FXOS8700Q_M_CTRL_REG1 0x5B
#define FXOS8700Q_M_CTRL_REG2 0x5C
#define FXOS8700Q_WHOAMI_VAL 0xC7

#define PI 3.14159265

WiFiInterface *wifi;
I2C i2c(PTD9,PTD8);
int m_addr = FXOS8700CQ_SLAVE_ADDR1;
volatile int message_num = 0;
volatile int arrivedcount = 0;
volatile bool closed = false;

const char* topic = "Mbed";
void FXOS8700CQ_readRegs(int addr, uint8_t * data, int len);
void FXOS8700CQ_writeRegs(uint8_t * data, int len);

void getAcc();
void getAddr(Arguments *in, Reply *out);
void query(Arguments *in,Reply *out);
RPCFunction rpcquery(&query, "query");

RawSerial pc(USBTX, USBRX);
RawSerial xbee(D12, D11);

EventQueue queue(32 * EVENTS_EVENT_SIZE);
Thread t;
Thread t2;
Timer timer1;
Thread mqtt_thread;
EventQueue mqtt_queue;
float ts=0.5;
float prev_angle;
int times=0;
int counttime=0;
int isasked=0;
EventQueue queue2(32 * EVENTS_EVENT_SIZE);
int timearr[20];
int init_flag=0;
float init_angle=0;
void xbee_rx_interrupt(void);
void xbee_rx(void);
void reply_messange(char *xbee_reply, char *messange);
void check_addr(char *xbee_reply, char *messenger);
void messageArrived(MQTT::MessageData& md) {
      MQTT::Message &message = md.message;
      
      char msg[300];
      sprintf(msg, "Message arrived: QoS%d, retained %d, dup %d, packetID %d\r\n", message.qos, message.retained, message.dup, message.id);
      printf(msg);
      wait_ms(1000);
      char payload[300];
      sprintf(payload, "Payload %.*s\r\n", message.payloadlen, (char*)message.payload);
      printf(payload);
      ++arrivedcount;
}

/*void publish_message(MQTT::Client<MQTTNetwork, Countdown>* client) {

      MQTT::Message message;
      char buff[100];
      sprintf(buff, "FXOS8700Q ACC: X=%1.4f Y=%1.4f Z=%1.4f");
      message.qos = MQTT::QOS0;
      message.retained = false;
      message.dup = false;
      message.payload = (void*) buff;
      message.payloadlen = strlen(buff) + 1;
      int rc = client->publish(topic, message);

      printf("rc:  %d\r\n", rc);
      printf("Puslish message: %s\r\n", buff);
}*/

void publish_message(MQTT::Client<MQTTNetwork, Countdown>* client) {

   while (true) {

      MQTT::Message message;

      char buff[100];

      FXOS8700CQ_readRegs(FXOS8700Q_OUT_X_MSB, res, 6);

      acc16 = (res[0] << 6) | (res[1] >> 2);

      if (acc16 > UINT14_MAX/2)

         acc16 -= UINT14_MAX;

      t[0] = ((float)acc16) / 4096.0f;

      acc16 = (res[2] << 6) | (res[3] >> 2);

      if (acc16 > UINT14_MAX/2)

         acc16 -= UINT14_MAX;

      t[1] = ((float)acc16) / 4096.0f;

      acc16 = (res[4] << 6) | (res[5] >> 2);

      if (acc16 > UINT14_MAX/2)

         acc16 -= UINT14_MAX;

      t[2] = ((float)acc16) / 4096.0f;

      sprintf(buff, "%1.4f %1.4f %1.4f \r\n",t[0],t[1],t[2]);

      message.qos = MQTT::QOS0;

      message.retained = false;

      message.dup = false;

      message.payload = (void*) buff;

      message.payloadlen = strlen(buff) + 1;

      int rc = client->publish(topic, message);

      //printf("rc:  %d\r\n", rc);

      printf("%s\r\n", buff);

      //if (closed) break;

      wait(ts);

   }

}

void close_mqtt() {
      closed = true;
}

int main(){
    pc.baud(9600);
    uint8_t datas[2] ;
    // Enable the FXOS8700Q
    FXOS8700CQ_readRegs( FXOS8700Q_CTRL_REG1, &datas[1], 1);
    datas[1] |= 0x01;
    datas[0] = FXOS8700Q_CTRL_REG1;
    FXOS8700CQ_writeRegs(datas, 2);
   
    char xbee_reply[4];
    // XBee setting
    //pc.printf("start xbee\r\n");
    xbee.baud(9600);
    xbee.printf("+++");
    xbee_reply[0] = xbee.getc();
    xbee_reply[1] = xbee.getc();
    if(xbee_reply[0] == 'O' && xbee_reply[1] == 'K'){
    pc.printf("enter AT mode.\r\n");
    xbee_reply[0] = '\0';
    xbee_reply[1] = '\0';
    }
    xbee.printf("ATMY 0x296\r\n");
    reply_messange(xbee_reply, "setting MY : 0x296");

    xbee.printf("ATDL 0x196\r\n");
    reply_messange(xbee_reply, "setting DL : 0x196");
	
	xbee.printf("ATID 0x2\r\n");
    reply_messange(xbee_reply, "setting PAN ID : 0x2");

    xbee.printf("ATWR\r\n");
    reply_messange(xbee_reply, "write config");

    xbee.printf("ATMY\r\n");
    check_addr(xbee_reply, "MY");

    xbee.printf("ATDL\r\n");
    check_addr(xbee_reply, "DL");

    xbee.printf("ATCN\r\n");
    reply_messange(xbee_reply, "exit AT mode");
    xbee.getc();

    // start
    pc.printf("start\r\n");
   

    wifi = WiFiInterface::get_default_instance();
    if (!wifi) {
        printf("ERROR: No WiFiInterface found.\r\n");
        return -1;
    }

    printf("\nConnecting to %s...\r\n", MBED_CONF_APP_WIFI_SSID);
    int ret = wifi->connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD, NSAPI_SECURITY_WPA_WPA2);
    if (ret != 0) {
        printf("\nConnection error: %d\r\n", ret);
        return -1;
    }

    NetworkInterface* net = wifi;
    MQTTNetwork mqttNetwork(net);
    MQTT::Client<MQTTNetwork, Countdown> client(mqttNetwork);

    
    const char* host = "192.168.43.101";
    printf("Connecting to TCP network...\r\n");
    int rc = mqttNetwork.connect(host, 1883);
    if (rc != 0) {
        printf("Connection error.");
        return -1;
    }
    printf("Successfully connected!\r\n");
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = "Mbed";

    if ((rc = client.connect(data)) != 0){
        printf("Fail to connect MQTT\r\n");
    }
    if (client.subscribe(topic, MQTT::QOS0, messageArrived) != 0){
        printf("Fail to subscribe\r\n");
    }

    t.start(callback(&queue, &EventQueue::dispatch_forever));
    t2.start(callback(&queue2,&EventQueue::dispatch_forever));
    
    ts=0.5;
    timer1.start();
    queue2.call(getAcc);
    //queue.call(xbee_rx);
    xbee.attach(xbee_rx_interrupt, Serial::RxIrq);

    int num = 0;
    while (num != 5) {
        client.yield(100);
        ++num;
    }
   
    mqtt_thread.start(callback(&mqtt_queue, &EventQueue::dispatch_forever));
    
    return 0;
	
}

void xbee_rx_interrupt(void)
{
  xbee.attach(NULL, Serial::RxIrq); // detach interrupt
  queue.call(&xbee_rx);
}

void xbee_rx(void){

    char buf[100] = {0};
    char outbuf[100] = {0};

    if(xbee.readable()){
        
        for (int i=0; ; i++) {
            char recv = xbee.getc();
            if (recv == '\r') {
                break;
            }
            buf[i] = pc.putc(recv);
            //buf[i] = recv;
        }
        
        //pc.printf("%s",buf);
        RPC::call(buf, outbuf);

        //pc.printf("%s\r\n", outbuf);
       // wait(0.1);
    }
 
    xbee.attach(xbee_rx_interrupt, Serial::RxIrq); // reattach interrupt
}

void reply_messange(char *xbee_reply, char *messange){
  xbee_reply[0] = xbee.getc();
  xbee_reply[1] = xbee.getc();
  xbee_reply[2] = xbee.getc();
  if(xbee_reply[1] == 'O' && xbee_reply[2] == 'K'){
    pc.printf("%s\r\n", messange);
    xbee_reply[0] = '\0';
    xbee_reply[1] = '\0';
    xbee_reply[2] = '\0';
  }
}

void check_addr(char *xbee_reply, char *messenger){
  xbee_reply[0] = xbee.getc();
  xbee_reply[1] = xbee.getc();
  xbee_reply[2] = xbee.getc();
  xbee_reply[3] = xbee.getc();
  pc.printf("%s = %c%c%c\r\n", messenger, xbee_reply[1], xbee_reply[2], xbee_reply[3]);
  xbee_reply[0] = '\0';
  xbee_reply[1] = '\0';
  xbee_reply[2] = '\0';
  xbee_reply[3] = '\0';
}
void getAcc() {
    
    
   

    while(1){
        int16_t acc16;
        float t[3];
        uint8_t res[6];
        float angle=90;
        
        FXOS8700CQ_readRegs(FXOS8700Q_OUT_X_MSB, res, 6);

        acc16 = (res[0] << 6) | (res[1] >> 2);
        if (acc16 > UINT14_MAX/2)
            acc16 -= UINT14_MAX;
        t[0] = ((float)acc16) / 4096.0f;

        acc16 = (res[2] << 6) | (res[3] >> 2);
        if (acc16 > UINT14_MAX/2)
            acc16 -= UINT14_MAX;
        t[1] = ((float)acc16) / 4096.0f;

        acc16 = (res[4] << 6) | (res[5] >> 2);
        if (acc16 > UINT14_MAX/2)
            acc16 -= UINT14_MAX;
        t[2] = ((float)acc16) / 4096.0f;

        /*if(!init_flag){
            init_angle=atan(t[2]/sqrt(t[0]*t[0]+t[1]*t[1]));
            prev_angle=init_angle;
            init_flag=1;
        }*/
        //angle = atan(t[2]/sqrt(t[0]*t[0]+t[1]*t[1]));
		
		float xangle = atan(t[0] / (sqrt(pow(t[1],2) + pow(t[2],2))));
        float yangle = atan(t[1] / (sqrt(pow(t[0],2) + pow(t[2],2))));
        float zangle = atan((sqrt(pow(t[0],2) + pow(t[1],2))) / t[2]);

        float Pitch = xangle * 180 / PI;
        float Roll = yangle * 180 / PI;
        float Yaw = zangle * 180 / PI;

        //pc.printf("%1.3f\r\n",angle);
        //printf("%1.3f\r\n",(init_angle-angle)*180/3.14159265358);
        if ( (Pitch >= 45.0) || (Roll >= 45.0) || (Yaw >= 45.0) ){
            ts=0.1;
            timer1.reset();
        }
        if(timer1.read_ms()>1000){
			ts=0.5;
        }   
        //prev_angle=angle;
        /*pc.printf("FXOS8700Q ACC: X=%1.4f(%x%x) Y=%1.4f(%x%x) Z=%1.4f(%x%x)",\
                t[0], res[0], res[1],\
                t[1], res[2], res[3],\
                t[2], res[4], res[5]\
        );*/
        /*if(isasked)
            times=0;
        else*/
        
        times++;
        if(times>=50)
            times=0;

        wait(ts);
        
    }
    
    
}

void getAddr(Arguments *in, Reply *out) {
   uint8_t who_am_i, data[2];
   FXOS8700CQ_readRegs(FXOS8700Q_WHOAMI, &who_am_i, 1);
   pc.printf("Here is %x", who_am_i);

}

void FXOS8700CQ_readRegs(int addr, uint8_t * data, int len) {
   char t = addr;
   i2c.write(m_addr, &t, 1, true);
   i2c.read(m_addr, (char *)data, len);
}

void FXOS8700CQ_writeRegs(uint8_t * data, int len) {
   i2c.write(m_addr, (char *)data, len);
}
void query(Arguments *in,Reply *out){

    pc.printf("%d\r\n",times);   

    char ss[3];
    sprintf(ss,"%02d",times);
    xbee.printf("%s",ss);   
    times=0;
    counttime++;
}   