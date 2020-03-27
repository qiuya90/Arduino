include "sim800c_onenet.h"

const char* DEV_ID = "587254975"; //设备ID
const char* DEV_PRO_ID = "324049"; //产品ID
const char* DEV_KEY = "dEWNtkuq3awc9f4lZTRMXM3gXG8="; //master API KEY

sim800c sim800c(16,17);

long lastMsg = 0;
char tmp[50];
String command;
int light_flag = 0;
int count = 0;
int s = 0;

#define LED 2

void setup()
{
  Serial.begin(9600);
  sim800c.ssbegin(9600);

  pinMode(LED, OUTPUT);

  do {
    s = sim800c.initTCP();
  } while (s = 0);
  delay(2000);
  sim800c.MQTTConnect(DEV_ID, DEV_PRO_ID, DEV_KEY);
  Serial.println("……………………………………………………");
  delay(2000);
  //public_data();
}

void loop() {

  command = sim800c.readServerResponse();//receive data from Onenet
  
  if (command.indexOf("1") != -1) {
    Serial.println("LED ON!");
    light_flag = 1;
    digitalWrite(LED, HIGH);   // Turn the LED on (Note that LOW is the voltage level
    snprintf(tmp, sizeof(tmp), "{\"light\":%d ,\"humi\":%d}", light_flag, random(30, 90));
    sim800c.public_data(tmp);
  }
  else if (command.indexOf("0") != -1) {
    Serial.println("LED OFF!");
    light_flag = 0;
    digitalWrite(LED, LOW);  // Turn the LED off by making the voltage HIGH
    snprintf(tmp, sizeof(tmp), "{\"light\":%d ,\"humi\":%d}", light_flag, random(30, 90));
    sim800c.public_data(tmp);
  }

  long now = millis();
  if (now - lastMsg > 60000) {
    lastMsg = now;
    count++;
    sim800c.MQTTConnect(DEV_ID, DEV_PRO_ID, DEV_KEY, 0, 0, 0, 0, 1);
    Serial.println("……………………………………………………");
    if (count >= 3) {
      snprintf(tmp, sizeof(tmp), "{\"light\":%d ,\"humi\":%d}", light_flag, random(30, 90));
      sim800c.public_data(tmp);
      count = 0;
    }
  }
}



