//  个人博客：http://blog.sina.com.cn/qiuya90
// 原理为利用ios12 支持的捷径软件和esp8266的http服务器
// 捷径进行http访问指定ip，esp8266检测到特定ip进行相应的io操作
// 2019.1.26 支持微信配网和安卓配网（可显示IP地址便于siri捷径添加ip）  安卓软件下载地址： https://fir.im/sc1t
#include <ESP8266WiFi.h>
#define LED 2
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
void setup() {
  Serial.begin(115200);
  delay(100);
  pinMode(2, OUTPUT);
  digitalWrite(2, 1);
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
  Serial.println("To set GPIO 0 high, type:");
  Serial.print(WiFi.localIP());
  Serial.println("/gpio/10");
  Serial.println("To set GPIO 0 low, type:");
  Serial.print(WiFi.localIP());
  Serial.println("/gpio/11");
  Serial.println("To toggle GPIO 0, type:");
  Serial.print(WiFi.localIP());
  Serial.println("/gpio/xxxxx");
}
void loop() {
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    delay(100);
    return;
  }
  String req = client.readStringUntil('\r');              //做为每一次指令的结束字符判断，因此指令末尾要有该结束符，可用16进制0x0D或者字符回车做为结束符
  Serial.println(req);
  client.flush();
  // Match the request
  int val;          
  //      下面开始判断ip进行动作指令
  if (req.indexOf("/gpio/10") != -1){
    digitalWrite(LED, 0);
    Serial.println("打开成功！");
    Serial.println(req.indexOf("/gpio/10"));
    }
  else if (req.indexOf("/gpio/11") != -1){
    digitalWrite(LED, 1);
    Serial.println("关闭成功！");
    Serial.println(req.indexOf("/gpio/11"));
    }
  else {
    Serial.println("invalid request");
    client.print("HTTP/1.1 404\r\n");
    client.stop();
    return;
  }
  client.flush();
  // Prepare the response
  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nWelcome ivan space! ";
  s += "</html>\n";
  // Send the response to the client
  client.print(s);
  delay(1);
  Serial.println("Client disonnected");
}
