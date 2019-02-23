#if defined ESP32
#include <WiFi.h>
#elif defined ESP8266
#include <ESP8266WiFi.h>
#else
#error "Only support espressif esp32/8266 chip"
#endif
#include "bigiot.h"

#define WIFI_TIMEOUT 30000

//////////////////////////////////////////////////////////////////////////////////////////////手动配网
/*
const char *ssid = "CMCC-YGf3";
const char *passwd = "xxxxxxx";
*/
//////////////////////////////////////////////////////////////////////////////////////////////

const char *id = "xxxx";              //platform device id
const char *apikey = "xxxxxxxx";      //platform device api key
const char *usrkey = "xxxxxxxx";           //platform user key , if you are not using encrypted login,you can leave it blank

#define SERVER_WECHAT_KEY       "SCU44912T4cffc7b4788f46f8cbefded6ddf577025c6b6c6e1943d"            //Third-party WeChat interface apikey，See http://sc.ftqq.com/3.version

#define LED_PIN 2                              // led pin

BIGIOT bigiot;

//中文 开灯 Uincode码
#define TURN_ON     "u5f00u706f"
//中文 关灯 Uincode码
#define TURN_OFF    "u5173u706f"

//////////////////////////////////////////////////////////////////////////////////////////////////自动配网
//WiFiServer server(80);  // 服务器端口号
bool autoConfig()       ////自动配网
{
  WiFi.begin();
  for (int i = 0; i < 10; i++)
  {
    int wstatus = WiFi.status();
    if (wstatus == WL_CONNECTED)
    {
      Serial.println("AutoConfig Success");
      Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
      Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());
      WiFi.printDiag(Serial);
      return true;
      //break;
    }
    else
    {
      Serial.print("AutoConfig Waiting......");
      Serial.println(wstatus);
      delay(1000);
    }
  }
  Serial.println("AutoConfig Faild!" );
  return false;
  //WiFi.printDiag(Serial);
}

void smartConfig()
{
  WiFi.mode(WIFI_STA);
  Serial.println("\r\nWait for Smartconfig");
  WiFi.beginSmartConfig();
  while (1)
  {
    Serial.print(".");
    digitalWrite(LED_PIN, 0);
    delay(500);
    digitalWrite(LED_PIN, 1);
    delay(500);
    if (WiFi.smartConfigDone())
    {
      Serial.println("SmartConfig Success");
      Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
      Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());
      break;
    }
  }
}
////////////////////////////////////////////////////////////////////////////////////////

void eventCallback(const int devid, const int comid, const char *comstr)
{
    // You can handle the commands issued by the platform here.
    Serial.printf("Received[%d] - [%d]:%s \n", devid, comid, comstr);
    switch (comid) {
    case STOP:
        digitalWrite(LED_PIN, HIGH);
        bigiot.FeedBackResult("ESP8266 LED OFF!");/////////////////////////////////////反馈结果在这
        delay(100);
        SendToWechat("灯关啦！");
        break;
    case PLAY:
        digitalWrite(LED_PIN, LOW);
        bigiot.FeedBackResult("ESP8266 LED ON!");/////////////////////////////////////反馈结果在这
        delay(100);
        SendToWechat("灯开啦！");
        break;
    case OFFON:
        break;
    case MINUS:
        break;
    case UP:
        break;
    case PLUS:
        break;
    case LEFT:
        break;
    case PAUSE:
        break;
    case RIGHT:
        break;
    case BACKWARD:
        break;
    case DOWN:
        break;
    case FPRWARD:
        break;
    case CUSTOM:
        // 当收到中文开灯时候将led输出低电平
        if (!strcmp(comstr, TURN_ON)) {
            digitalWrite(LED_PIN, LOW);
        // 当收到中文关灯时候将led输出高电平
        } else if (!strcmp(comstr, TURN_OFF)) {
            digitalWrite(LED_PIN, HIGH);
        }
        break;
    default:
        break;
    }
}

void disconnectCallback(BIGIOT &obj)
{
    // When the device is disconnected to the platform, you can handle your peripherals here
    Serial.print(obj.deviceName());
    Serial.println("  disconnect");
}

String deviceName;
void connectCallback(BIGIOT &obj)
{
    if(String(obj.deviceName()) == "u5367u5ba4u7684u706f"){
        deviceName = "卧室的灯";
      }else{
      deviceName = String(obj.deviceName());}
    // When the device is connected to the platform, you can preprocess your peripherals here
    String content = "[" + String(millis()) + "]" + "_" + deviceName + "_已连接啦";
    String title = "来自_" + deviceName + "_发来的消息";
    
    // Or you can send wechat message
    ServerChan cat(SERVER_WECHAT_KEY);
    if (!cat.sendWechat(title, content)) {
        Serial.println("Send fail");
    }   
    Serial.print(obj.deviceName());
    Serial.println("  connect");
}

void SendToWechat(String wechatmsg)
{
    // When the device is connected to the platform, you can preprocess your peripherals here
    String content = "[" + String(millis()) + "]" + "_" + deviceName + "_" + wechatmsg;
    String title = "来自_" + deviceName + "_发来的消息";
    
    //send wechat message
    ServerChan cat(SERVER_WECHAT_KEY);
    if (!cat.sendWechat(title, content)) {
        Serial.println("Send fail");
    }  
}


void setup()
{
    Serial.begin(115200);

    delay(100);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
////////////////////////////////////////////////////////////////////////////////////手动配网
/*
    WiFi.begin(ssid, passwd);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.print("Connect ssid fail");
        while (1);
    }
*/
/////////////////////////////////////////////////////////////////////////////////////自动配网

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  //smartConfig();  //APP智能配网
  if (!autoConfig())
  {
    Serial.println("Start module");
    smartConfig();
  }
  
  delay(500);
  Serial.println("");
  Serial.println("WiFi connected");
  // Start the server
  //server.begin();
  //Serial.println("Server started @ ");
  // Print the IP address & instructions
  Serial.println(WiFi.localIP());
/////////////////////////////////////////////////////////////////////////////

    //Regist platform command event hander
    bigiot.eventAttach(eventCallback);

    //Regist device disconnect hander
    bigiot.disconnectAttack(disconnectCallback);

    //Regist device connect hander
    bigiot.connectAttack(connectCallback);

    // Login to bigiot.net
    if (!bigiot.login(id, apikey, usrkey)) {
        Serial.println("Login fail");
        while (1);
    }
}


void loop()
{
    static uint64_t last_upload_time = 0;
    static uint64_t last_wifi_check_time = 0;

    if (WiFi.status() == WL_CONNECTED) {
        //Wait for platform command release
        bigiot.handle();
    } else {
        uint64_t now = millis();
        // Wifi disconnection reconnection mechanism
        if (now - last_wifi_check_time > WIFI_TIMEOUT) {
            Serial.println("WiFi connection lost. Reconnecting...");
            WiFi.reconnect();
            last_wifi_check_time = now;
        }
    }
}
