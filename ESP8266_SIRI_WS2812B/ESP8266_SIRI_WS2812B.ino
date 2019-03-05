//  个人博客：http://blog.sina.com.cn/qiuya90
// 原理为利用ios12 支持的捷径软件和esp8266的http服务器
// 捷径进行http访问指定ip，esp8266检测到特定ip进行相应的io操作
// 2019.1.26 支持微信配网和安卓配网（可显示IP地址便于siri捷径添加ip）  安卓软件下载地址： https://fir.im/sc1t
#include <ESP8266WiFi.h>
#include <FastLED.h>   //加入WS2812B灯条头文件
#include <String.h>

#define LED 2
#define LED_PIN 4  //定义灯条数据口
#define NUM_LEDS 8  //定义灯条的灯个数，是个数！
#define WIFI_TIMEOUT 30000   //定义网络连接超时时间

int R = 0, G = 0, B = 0;  //三色初始化
char colorinfo[100];
int setmode;
int j=0,k=0,l=0,n=5;                  //设置闪烁到1s，因为blink为100ms
int cR,cG,cB;
bool RGBreverse = true;

// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;        // will store last time LED was updated
// constants won't change:
const long interval = 100;           // interval at which to blink (milliseconds)

CRGB leds[NUM_LEDS];  //初始化灯条

// 以上烧写程序配网方式可以换成微信扫码配网，等等，可看说明文档
WiFiServer server(80);  // 服务器端口号

bool autoConfig()
{
  WiFi.begin();
  for (int i = 0; i < 20; i++)
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
    digitalWrite(LED, 0);
    delay(500);
    digitalWrite(LED, 1);
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

void ColorChange(char* color)               //判断输入的颜色
{
  if(strstr(color,"LightOff") != NULL){
    R = 0; G = 0; B = 0;
  }
  else if(strstr(color,"LightRED") != NULL){
    R = 255; G = 0; B =0;
  }
  else if(strstr(color,"LightGREEN") != NULL){
    R = 0; G = 255; B = 0;
  }
  else if(strstr(color,"LightBLUE") != NULL){
    R = 0; G = 0; B = 255;
  }
  else if(strstr(color,"LightCHERRY") != NULL){
    R = 244; G = 154; B = 193;
  }
  else if(strstr(color,"LightSKYBLUE") != NULL){
    R = 0; G = 191; B = 255;
  }
  else if(strstr(color,"LightAURORAPURPLE") != NULL){
    R = 85; G = 26; B = 180;
  }
  else if(strstr(color,"LightLEMONYELLOW") != NULL){
    R = 255; G = 255; B = 0;
  }
  else if(strstr(color,"LightWHITE1")||strstr(color,"LightOn") != NULL){
    R = 255; G = 255; B = 255;
  }
  else if(strstr(color, "LightBlink1") != NULL){         //按当前颜色闪烁
    //R = 0; G = 0; B = 0;
    cR = R; cG = G; cB = B;
    setmode = 1;
  }
  else if(strstr(color, "LightBlink2") != NULL){         //重新渐变
    R = 0; G = 0; B = 0;
    setmode = 2;
  }
  else if(strstr(color, "LightBlink3") != NULL){         //重新渐变
    R = 0; G = 0; B = 0;
    setmode = 3;
  }
}

void LedBlink(int setmode){                             //闪烁渐变程序
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
      // save the last time you blinked the LED
      previousMillis = currentMillis;
      switch(setmode){
      case 1:
          j++;
          if(j >= 10){
            if(RGBreverse == true){
            //cR = R; cG = G; cB = B;
            R = 0; G = 0; B = 0;
            RGBreverse = !RGBreverse;
            }
            else{
            R = cR; G = cG; B = cB;
            RGBreverse = !RGBreverse;
            }
            j = 0;
            for(int i=0; i <= NUM_LEDS; i++){
                leds[i] = CRGB(R, G, B);
            }
          }
          break;
      case 2:
          l++;
          if(l >= 3){
            R=(R+45)%255; G=(G+15)%255; B=(B+5)%255;
            for(int i=0; i <= NUM_LEDS; i++){
                leds[i] = CRGB(R, G, B);
              }
            l = 0;
          }
          break;
      case 3:
          if(k ==0){
          leds[0] = CRGB(R, G, B);
          }else{
            int i=k;
            while(i >= 1){
              leds[i] = leds[i-1];
              i--;
            }
            R=(R+5)%255; G=(G+15)%255; B=(B+45)%255;
            leds[0] = CRGB(R, G, B);
          }
          k++;
          if(k > 8){k = 0;}
          break;
    }

  }
}

void SingleColor() {                                     //单一颜色执行程序
    ColorChange(colorinfo);
    for(int i=0; i <= NUM_LEDS; i++){
      leds[i] = CRGB(R, G, B);
    }
    FastLED.show();
    setmode = 0;
}


void setup() {
  //ESP.eraseConfig();            //擦除SmartConfig的记录，重新配网
  //delay(100);
  
  Serial.begin(115200);
  delay(100);
  pinMode(2, OUTPUT);//设定8266led输出模式
  digitalWrite(2, 1);//设定8266led初始状态

  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);  //初始化WS2812B的灯条参数

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
  server.begin();
  Serial.println("Server started @ ");
  // Print the IP address & instructions
  Serial.println(WiFi.localIP());
  //打印出IP地址，后期可以制作显示器来外部硬件显示ip
  Serial.println("To control GPIO, open your web browser.");
  Serial.println("To set GPIO 2 high, type:");
  Serial.print(WiFi.localIP());
  Serial.println("LightOff");
  Serial.println("To set GPIO 2 low, type:");
  Serial.print(WiFi.localIP());
  Serial.println("LightOff");
  Serial.println("To toggle GPIO 2, type:");
  Serial.print(WiFi.localIP());
  //Serial.println("/gpio/xxxxx");
}
void loop() {
  static uint64_t last_wifi_check_time = 0;
  unsigned long currentMillis = millis();
    
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    //delay(100);
    //Serial.println(setmode);
    if(setmode == 0){}
    else{
      LedBlink(setmode);
      }
    delay(10);
    FastLED.show();
    return;
  }
  String req = client.readStringUntil('\r');              //做为每一次指令的结束字符判断，因此指令末尾要有该结束符，可用16进制0x0D或者字符回车做为结束符
  Serial.println(req);
  bool Corre = RequestInfo(req);
  if(Corre == false){
    client.print("HTTP/1.1 404\r\n");
    client.stop();
  }
  else{
    }
    
  client.flush();
  // Prepare the response
  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nWelcome ivan space! ";
  s += "</html>\n";
  // Send the response to the client
  client.print(s);
  delay(20);
  Serial.println("Client disonnected");
  
  if (WiFi.status() != WL_CONNECTED){
     uint64_t now = millis();
     // Wifi disconnection reconnection mechanism
     if (now - last_wifi_check_time > WIFI_TIMEOUT) {
      Serial.println("WiFi connection lost. Reconnecting...");
      WiFi.reconnect();
      last_wifi_check_time = now;
      }
  }
}

bool RequestInfo(String req){
  strcpy(colorinfo, req.c_str());       //将String转化为char
  // Match the request
  //下面开始判断ip进行动作指令
  if (req.indexOf("/LightOn") != -1){
    digitalWrite(LED, 0);
    SingleColor();
    Serial.println("开灯成功！");
    return true;
    }
  else if (req.indexOf("/LightOff") != -1){
    digitalWrite(LED, 1);
    SingleColor();
    Serial.println("关闭成功！");
    return true;
    }
  else if (req.indexOf("/LightRED") != -1){              //对WS2812B灯条的操作“红”
    SingleColor();
    Serial.println("变为红色！");
    return true;
    }
  else if (req.indexOf("/LightGREEN") != -1){              //对WS2812B灯条的操作“绿”
    SingleColor();
    Serial.println("变为绿色！");
    return true;
    }
  else if (req.indexOf("/LightBLUE") != -1){              //对WS2812B灯条的操作“蓝”
    SingleColor();
    Serial.println("变为蓝色！");
    return true;
    }
  else if (req.indexOf("/LightWHITE1") != -1){              //对WS2812B灯条的操作“白”
    SingleColor();
    Serial.println("变为白色！");
    return true;
    }
    else if (req.indexOf("/LightCHERRY") != -1){              //对WS2812B灯条的操作“樱花粉”
    SingleColor();
    Serial.println("变为樱花粉！");
    return true;
    }
    else if (req.indexOf("/LightSKYBLUE") != -1){              //对WS2812B灯条的操作“天际蓝”
    SingleColor();
    Serial.println("变为天际蓝！");
    return true;
    }
    else if (req.indexOf("/LightAURORAPURPLE") != -1){              //对WS2812B灯条的操作“极光紫”
    SingleColor();
    Serial.println("变为极光紫！");
    return true;
    }
    else if (req.indexOf("/LightLEMONYELLOW") != -1){              //对WS2812B灯条的操作“柠檬黄”
    SingleColor();
    Serial.println("变为柠檬黄！");
    return true;
    }
    else if (req.indexOf("/LightWHITEADD") != -1){              //对WS2812B灯条的操作“白亮”
    R = 255; G = 255; B = 255;
    for(int i=0; i <= NUM_LEDS; i++){
      leds[i] = CRGB(0, 0, 0);
    }
    if(n<8){n += 2;}
    else{n=8;}
    for(int i=0; i <= n; i++){
      leds[i] = CRGB(R, G, B);
      //Serial.print(leds[i]);
      //Serial.print(" ");
    }
    FastLED.show();
    setmode = 0;
    Serial.println("变为白亮！");
    return true;
    }
    else if (req.indexOf("/LightWHITEMIN") != -1){              //对WS2812B灯条的操作“白暗”
    R = 255; G = 255; B = 255;
    for(int i=0; i <= NUM_LEDS; i++){
      leds[i] = CRGB(0, 0, 0);
    }
    if(n>=3){n -= 2;}
    else{n=1;}
    for(int i=0; i <= n; i++){
      leds[i] = CRGB(R, G, B);
      //Serial.print(leds[i]);
      //Serial.print(" ");
    }
    FastLED.show();
    setmode = 0;
    Serial.println("变为白暗！");
    return true;
    }
  else if (req.indexOf("/LightBlink") != -1){              //对WS2812B灯条的操作“闪变”
    ColorChange(colorinfo);
    }
  else {
    Serial.println("invalid request");
    return false;
  }
}
