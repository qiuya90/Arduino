﻿/////////////////////////////////////////////////////////////////
/*
  bigiot.cpp - Arduino library simplifies the use of connected BIGIOT platforms.
  Created by Lewis he on January 1, 2019.
*/
/////////////////////////////////////////////////////////////////

#include "bigiot.h"
#include <base64.h>
#if defined ESP32
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#elif defined ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#endif
#include <MD5Builder.h>
#include <ArduinoJson.h>

StaticJsonBuffer<1024> jsonBuffer;

/////////////////////////////////////////////////////////////////
BIGIOT::BIGIOT()
{
    _host = BIGIOT_PLATFORM_HOST;
    _port = BIGIOT_PLATFORM_PORT;
}

/////////////////////////////////////////////////////////////////
const char *BIGIOT::deviceName()
{
    return _devName.c_str();
}

/////////////////////////////////////////////////////////////////
String BIGIOT::deviceName() const
{
    return _devName;
}
/////////////////////////////////////////////////////////////////
void BIGIOT::setHeartFreq(uint32_t f)
{
    _heartFreq = f;
}

/////////////////////////////////////////////////////////////////
bool BIGIOT::isOnline()
{
    return _isLogin;
}
/////////////////////////////////////////////////////////////////
int BIGIOT::handle(void)
{
    int com = DISCONNECT;
    static uint64_t hearTimeStamp = millis();
    static uint64_t reconnectTimeStamp = 0;

    if (connected() && _isLogin) {
        if (WiFiClient::available()) {
            String pack = readStringUntil('\n');
            com = packetParse(pack);
        }
        if (millis() - hearTimeStamp > _heartFreq) {
            print(BIGIOT_PLATFORM_8282_HATERATE_PACK);
            //print("{\"M\":\"say\",\"ID\":\"ALL\",\"C\":\"Hello\"}\n");/////////////////////////////////////反馈结果在这
            hearTimeStamp = millis();
        }
    } else {
        _isLogin = false;
        if (_disconnectCallback && !_isCall) {
            _isCall = true;
            _disconnectCallback(*this);
        }
        if (_reconnect && millis() - reconnectTimeStamp > REC_TIMEOUT) {
            DEBUG_BIGIOTCIENT("RECONNECT :%s", _devName.c_str());
            reconnectTimeStamp = millis();
            loginToBigiot();
        }
    }
    return com;
}
/////////////////////////////////////////////////////////////////
int BIGIOT::FeedBackResult(String msg)
{
  int temp = DISCONNECT;
    if (connected() && _isLogin) {
            print("{\"M\":\"say\",\"ID\":\"ALL\",\"C\":\"");/////////////////////////////////////反馈结果在这
            print(msg);
            print("\"}\n");
            Serial.printf("Send!");
    }
    return temp;
}
/////////////////////////////////////////////////////////////////
int BIGIOT::packetParse(String pack)
{
    DEBUG_BIGIOTCIENT("%s", pack.c_str());
    jsonBuffer.clear();
    JsonObject &root = jsonBuffer.parseObject(pack);
    if (!root.success()) {
        return 0;
    }
    const char *m = (const char *)root["M"];
    if (!strcmp(m, "say")) {
        const char *s = (const char *)root["C"];
        for (int i = 0; i < PLATFORM_ARRAY_SIZE(platform_command); ++i) {
            if (!strcmp(s, platform_command[i])) {
                if (_eventCallback) {
                    _eventCallback(_dev.toInt(), i, platform_command[i]);
                }
                return i + 1;
            }
        }
        if (_eventCallback) {
            _eventCallback(_dev.toInt(), CUSTOM, s);
        }
    } else if (!strcmp(m, "checkout")) {
        const char *r = (const char *)root["IP"];
        DEBUG_BIGIOTCIENT("RECV CHECKOUT:%s\n", r);
    }
    return 0;
}

/////////////////////////////////////////////////////////////////
int BIGIOT::loginParse(String pack)
{
    JsonObject &root = jsonBuffer.parseObject(pack);
    if (!root.success())
        return INVALD;
    const char *m = (const char *)root["M"];
    if (!strcmp(m, "WELCOME TO BIGIOT")) {
        return BIGIOT_LOGINT_WELCOME;
    } else if (!strcmp(m, "checkinok")) {
        _isCall = false;
        _devName = (const char *)root["NAME"];
        DEBUG_BIGIOTCIENT("CHECKOK Device Name:%s\n", _devName.c_str());
        return BIGIOT_LOGINT_CHECK_IN;
    } else if (!strcmp(m, "token") && _usrKey.length()) {
        MD5Builder md5;
        md5.begin();
        md5.add(String((const char *)root["K"]) + _usrKey);
        md5.calculate();
        _token = md5.toString();
        print(getLoginPacket(_token));
        return BIGIOT_LOGINT_TOKEN;
    }
    return INVALD;
}

/////////////////////////////////////////////////////////////////
bool BIGIOT::loginToBigiot(void)
{
    if (!connect(_host.c_str(), _port)) {
        DEBUG_BIGIOTCIENT("CONNECT HOST FAIL\n");
        return false;
    }
    print(getLoginPacket(_key));
    uint64_t timeStamp = millis();
    for (;;) {
        if (millis() - timeStamp > 5000) {
            DEBUG_BIGIOTCIENT("LOGIN_TIMEOUT\n");
            print(getLogoutPacket());
            stop();
            return false;
        }
        if (WiFiClient::available()) {
            String line = readStringUntil('\n');
            DEBUG_BIGIOTCIENT("RECV:%s\n", line.c_str());
            if (loginParse(line) == BIGIOT_LOGINT_CHECK_IN) {
                _isLogin = true;
                if (_connectCallback) {
                    _connectCallback(*this);
                }
                return true;
            }
        }
        delay(100);
    }
}

/////////////////////////////////////////////////////////////////
bool BIGIOT::login(const char *devId, const char *apiKey, const char *userKey, bool reconnect)
{
    _dev = devId;
    _key = apiKey;
    _usrKey = userKey;
    _reconnect = reconnect;
    int retry = 3;
    do {
        if (loginToBigiot())return true;
    } while (--retry);
    return false;
}

/////////////////////////////////////////////////////////////////
bool BIGIOT::operator == (BIGIOT &b)
{
    return (this == &b);
}

/////////////////////////////////////////////////////////////////

void BIGIOT::eventAttach(eventCallbackFunc f)
{
    _eventCallback = f;
}

/////////////////////////////////////////////////////////////////
void BIGIOT::disconnectAttack(generlCallbackFunc f)
{
    _disconnectCallback = f;
}

/////////////////////////////////////////////////////////////////
void BIGIOT::connectAttack(generlCallbackFunc f)
{
    _connectCallback = f;
}

/////////////////////////////////////////////////////////////////
String BIGIOT::getLoginPacket(String apiKey)
{
    String pack;
    jsonBuffer.clear();
    JsonObject &root = jsonBuffer.createObject();
    root["M"] = "checkin";
    root["ID"] = _dev;
    root["K"] = apiKey;
    root.printTo(pack);
    pack += "\n";
    DEBUG_BIGIOTCIENT("SEND LOGIN COMMAND:%s", pack.c_str());
    return pack;
}

/////////////////////////////////////////////////////////////////
String BIGIOT::getLogoutPacket(void)
{
    String pack;
    jsonBuffer.clear();
    JsonObject &root = jsonBuffer.createObject();
    root["M"] = "checkout";
    root["ID"] = _dev;
    if (_token.length())
        root["K"] = _token;
    else
        root["K"] = _key;
    root.printTo(pack);
    pack   += "\n";
    DEBUG_BIGIOTCIENT("SEND CHECKOUT COMMAND:%s", pack.c_str());
    return pack;
}

/////////////////////////////////////////////////////////////////
bool BIGIOT::sendAlarm(String method, String message)
{
    return sendAlarm(method.c_str(), message.c_str());
}

/////////////////////////////////////////////////////////////////
bool BIGIOT::sendAlarm(const char *method, const char *message)
{
    static uint64_t last_send_time = 0;

    if (!_isLogin || !method || !message)return false;

    if (last_send_time && millis() -  last_send_time < BIGIOT_PLATFORM_ALARM_INTERVAL)
        return false;

    if (!strcmp(method, "email") ||
            !strcmp(method, "qq") ||
            !strcmp(method, "weibo")) {
        jsonBuffer.clear();
        JsonObject &root = jsonBuffer.createObject();
        root["M"] = "alert";
        root["C"] = message;
        root["B"] = method;
        String json;
        root.printTo(json);
        json += "\n";
        print(json);
        DEBUG_BIGIOTCIENT("Send:%s", json);
        last_send_time = millis();
        return true;
    }
    return  false;
}

/////////////////////////////////////////////////////////////////
// Note:需要在HTTP Header中增加API-Key来授权写入操作,  支持一次传送一幅图像数据；
// Note:目前限定相邻图像数据上传间隔须大于等于10s；
// Note:图片大小不大于100K；
// Note:图片格式jpg、png、gif。
bool BIGIOT::uploadPhoto( const char *id, const char *type, const char *filename, uint8_t *image, size_t size)
{
    char buff[512];
    char status[64] = {0};

    if (!id || !type || !image || !size || !filename || size > 100 * 1024)return false;


    if (strcmp(type, "jpeg") &&
            strcmp(type, "jpg")  &&
            strcmp(type, "png")  &&
            strcmp(type, "gif")) {
        DEBUG_BIGIOTCIENT("Error data type\n");
        return false;
    }

    WiFiClientSecure client;

    if (!client.connect(BIGIOT_PLATFORM_HOST, BIGIOT_PLATFORM_HTTPS_PORT)) {
        DEBUG_BIGIOTCIENT("Connection failed");
        return false;
    }
    const char *request_content = "--------------------------ef73a32d43e7f04d\r\n"
                                  "Content-Disposition: form-data; name=\"data\"; filename=\"%s.%s\"\r\n"
                                  "Content-Type: image/%s\r\n\r\n";

    const char *request_end = "\r\n--------------------------ef73a32d43e7f04d--\r\n";

    snprintf(buff, sizeof(buff), request_content, filename, type, type);

    uint32_t content_len = strlen(request_end) + strlen(buff) + size;
    String request = "POST /pubapi/uploadImg/did/";
    request += _dev;
    request += "/inputid/";
    request += id;
    request += " HTTP/1.1\r\n";
    request += "Host: ";
    request += BIGIOT_PLATFORM_HOST;
    request += "\r\n";
    request += "API-KEY: ";
    request += _key;
    request += "\r\n";
    request += "Content-Length: " + String(content_len) + "\r\n";
    request += "Content-Type: multipart/form-data; boundary=------------------------ef73a32d43e7f04d\r\n";
    request += "Expect: 100-continue\r\n";
    request += "\r\n";

    client.println(request);
    client.readBytesUntil('\r', status, sizeof(status));
    if (strcmp(status, "HTTP/1.1 100 Continue") != 0) {
        DEBUG_BIGIOTCIENT("Unexpected response: %s\n", status);
        client.stop();
        return false;
    }

    client.print(buff);
    content_len = size;
    size_t offset = 0;
    size_t ret = 0;
    while (1) {
        ret = client.write(image + offset, content_len);
        offset += ret;
        content_len -= ret;
        if (size == offset) {
            break;
        }
        delay(1);
    }
    client.print(request_end);
    client.find("\r\n");
    bzero(status, sizeof(status));
    client.readBytesUntil('\r', status, sizeof(status));
    if (strncmp(status, "HTTP/1.1 200 OK", strlen("HTTP/1.1 200 OK"))) {
        DEBUG_BIGIOTCIENT("Unexpected response: %s\n", status);
        client.stop();
        return false;
    }

    if (!client.find("\r\n\r\n")) {
        DEBUG_BIGIOTCIENT("Invalid response\n");
        client.stop();
        return false;
    }

    request = client.readStringUntil('\n');
    client.stop();

    char *str = strdup(request.c_str());
    if (!str) {
        return false;
    }
    char *start = strchr(str, '{');
    jsonBuffer.clear();
    JsonObject &root = jsonBuffer.parseObject(start);
    if (!root.success()) {
        free(str);
        return false;
    }
    bool retVal = !strcmp((const char *)root["R"], "0") ? false : true;
    free(str);
    return retVal;
}

/////////////////////////////////////////////////////////////////
bool BIGIOT::upload(String id, String data)
{
    return  upload(id.c_str(), data.c_str());
}

/////////////////////////////////////////////////////////////////
bool BIGIOT::upload(const char *id, const char *data)
{
    return upload(&id, &data, 1);
}

/////////////////////////////////////////////////////////////////
bool BIGIOT::upload(const char *id[], const char *data[], int len)
{
    if (!_isLogin || !data || !len)return false;
    jsonBuffer.clear();
    JsonObject &root = jsonBuffer.createObject();
    root["M"] = "update";
    root["ID"] = _dev;
    JsonObject &v = root.createNestedObject("V");
    for (int i = 0; i < len; ++i) {
        v[id[i]] = data[i];
    }
    String json;
    root.printTo(json);
    json += "\n";
    print(json);
    DEBUG_BIGIOTCIENT("Send:%s", json);
    return true;
}

/////////////////////////////////////////////////////////////////
bool loaction(String id, String longitude, String latitude)
{
    return loaction(id.c_str(), longitude.c_str(), latitude.c_str());
}

/////////////////////////////////////////////////////////////////
bool BIGIOT::loaction(const char *id, float longitude, float latitude)
{
    char longitudeBf[16], latitudeBf[16];
    if (!id)return false;
    snprintf(longitudeBf, sizeof(longitudeBf), "%f", longitude);
    snprintf(latitudeBf, sizeof(latitudeBf), "%f", latitude);
    return loaction(id, longitudeBf, latitudeBf);
}

/////////////////////////////////////////////////////////////////
bool BIGIOT::loaction(const char *id, const char *longitude, const char *latitude)
{
    char buff[128];
    if (!_isLogin)return false;
    jsonBuffer.clear();
    JsonObject &root = jsonBuffer.createObject();
    root["M"] = "update";
    root["ID"] = _dev;
    JsonObject &v = root.createNestedObject("V");
    snprintf(buff, sizeof(buff), "%s,%s", longitude, latitude);
    v[id] = buff;
    String json;
    root.printTo(json);
    json += "\n";
    print(json);
    DEBUG_BIGIOTCIENT("Send:%s", json);
    return true;
}

/////////////////////////////////////////////////////////////////
void xEamil::setEmailHost(const char *host, uint16_t port)
{
    _emailHost = host;
    _emailPort = port;
}

/////////////////////////////////////////////////////////////////
void xEamil::setRecipient(const char *email)
{
    _recipient = email;
}

/////////////////////////////////////////////////////////////////
bool xEamil::setSender(const char *user, const char *password)
{
    _emailUser = user;
    _emailPasswd = password;

    _base64User = base64::encode(_emailUser);
    _base64Pass = base64::encode(_emailPasswd);
    if (_base64User == "-FAIL-" || _base64Pass == "-FAIL-") {
        return false;
    }
    return true;
}

/////////////////////////////////////////////////////////////////
bool xEamil::sendEmail(const char *subject, const char *content)
{
    String ip = WiFiClient::localIP().toString();
    struct {
        int cnt;
        int skip;
        const char *prefix;
        const char *arg;
    }   stream[] = {
        {1, 0, "EHLO %s", ip.c_str()},
        {0, 0, "auth login",  NULL},
        {2, 0, NULL,  _base64User.c_str()},
        {2, 0, NULL,  _base64Pass.c_str()},
        {1, 0, "MAIL From: <%s>",  _emailUser.c_str()},
        {1, 0, "RCPT To: <%s>", _recipient.c_str()},
        {0, 0, "DATA",  NULL},
        {1, 1, "To: <%s>",  _recipient.c_str()},
        {1, 1, "From: <%s>",  _emailUser.c_str()},
        {1, 1, "Subject: %s\r\n",  subject},
        {2, 1, NULL,  content},
        {0, 0, ".",  NULL},
        {0, 0, "QUIT", NULL}
    };

    char buff[256];
    if (!connect(_emailHost.c_str(), _emailPort))
        return false;
    if (!emailRecv())
        return false;

    for (int i = 0; i < PLATFORM_ARRAY_SIZE(stream); ++i) {
        switch (stream[i].cnt) {
        case 0:
            DEBUG_BIGIOTCIENT("%s\n", stream[i].prefix);
            println(stream[i].prefix);
            break;
        case 1:
            snprintf(buff, sizeof(buff), stream[i].prefix, stream[i].arg);
            DEBUG_BIGIOTCIENT("%s\n", buff);
            println(buff);
            break;
        case 2:
            DEBUG_BIGIOTCIENT("%s\n", stream[i].arg);
            println(stream[i].arg);
            break;
        }
        if (!stream[i].skip) {
            if (!emailRecv())
                return false;
        }
    }
    stop();
    return true;
}

/////////////////////////////////////////////////////////////////
bool xEamil::sendEmail(String &subject, String &content)
{
    return sendEmail(subject.c_str(), content.c_str());
}

/////////////////////////////////////////////////////////////////
bool xEamil::emailRecv()
{
    uint8_t code;
    int loopCount = 0;
    while (!available()) {
        delay(1);
        loopCount++;
        if (loopCount > 10000) {
            stop();
            DEBUG_BIGIOTCIENT("\r\nxEamil Timeout\n");
            return 0;
        }
    }
    code = peek();
    Serial.print("MailRecv:");
    while (available()) {
        DEBUG_BIGIOT_WRITE(read());
    }
    if (code >= '4') {
        emailFail();
        return 0;
    }
    return 1;
}

/////////////////////////////////////////////////////////////////
void xEamil::emailFail()
{
    int loopCount = 0;
    println(F("QUIT"));
    while (!available()) {
        delay(1);
        loopCount++;
        if (loopCount > XEMAIL_RECV_TIMEOUT) {
            stop();
            DEBUG_BIGIOTCIENT("\r\nxEamil Timeout\n");
            return;
        }
    }
    DEBUG_BIGIOTCIENT("MailRecv:");
    while (available()) {
        DEBUG_BIGIOT_WRITE(read());
    }
    stop();
}

/////////////////////////////////////////////////////////////////
void ServerChan::setSCKEY(String &key)
{
    _sckey = key;
}

/////////////////////////////////////////////////////////////////
void ServerChan::setSCKEY(const char *key)
{
    _sckey = key;
}

/////////////////////////////////////////////////////////////////
bool ServerChan::sendWechat(String text, String desp)
{
    sendWechat(text.c_str(), desp.c_str());
}

/////////////////////////////////////////////////////////////////

bool ServerChan::sendWechat(const char *text, const char *desp)
{
    size_t size = strlen(text) + _sckey.length() + strlen(SERVERCHAN_LINK_FORMAT);
    if (desp) {
        size = strlen(desp) > SERVERCHAN_DESP_MAX_LENGTH ? SERVERCHAN_DESP_MAX_LENGTH : size + strlen(desp);
    }
    char *buff = nullptr;
    buff = new char[size]();
    if(buff == "")
    {
      return false;
      }
//    try {
//        buff = new char[size]();
//    } catch (std::bad_alloc) {
//        return false;
//    }

    snprintf(buff, size, SERVERCHAN_LINK_FORMAT, _sckey.c_str(), text);
    if (desp) {
        snprintf(buff, size, "%s&desp=%s", buff, desp);
    }

    for (char *p = buff; *p; p++) {
        if (*p == ' ')
            *p = '+';
    }

    DEBUG_BIGIOTCIENT("ServerChan Request:%s", buff);

    HTTPClient http;
    http.begin(buff);//esp8266 deprecated this method
    int err = http.GET();
    DEBUG_BIGIOTCIENT("[HTTP] GET.code: %d\n", err);
    http.end();
    delete [] buff;
    return err == HTTP_CODE_OK;
}
