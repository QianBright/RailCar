#include <analogWrite.h>

//wifi
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <esp_wifi.h>
#include <WiFiManager.h>

//闪存
#include <FS.h>
#include <SPIFFS.h>

//led
#include "ledWifi.h"

#define KEY_IO0 0
ledWifi LED_WIFI(32);

WiFiMulti wifiMulti;
WiFiManager wifiManager;
WebServer server(80);

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("////////////start////////////");
  Serial.println();

  pinMode(KEY_IO0, INPUT_PULLUP);

  // 启动闪存文件系统
  if (SPIFFS.begin()) {
    Serial.println("SPIFFS 开启.");
  } else {
    Serial.println("SPIFFS 开启失败.");
  }

  wifiManager.autoConnect("eDray_ID-000");

  //初始化网络服务器
  server.on("/LED-Control", handleLEDControl);
  server.onNotFound(handleUserRequest); // 处理其它网络请求

  // 启动网站服务
  server.begin();
  Serial.println("HTTP server started");
  Serial.println();
}

void loop(void) {
  server.handleClient();  //处理网络请求
  //Serial.print(".");

  //用于删除已存WiFi
  if (digitalRead(KEY_IO0) == LOW) {
    LED_WIFI.flash();
    delay(1000);
    wifiManager.resetSettings();
    Serial.println("WiFi Settings Cleared");
    delay(10);
    ESP.restart();  //复位esp32
  }
}

//LED
void handleLEDControl() {
  // 从浏览器发送的信息中获取PWM控制数值（字符串格式）
  String ledPwm = server.arg("ledPwm");

  // 将字符串格式的PWM控制数值转换为整数
  int ledPwmVal = ledPwm.toInt();

  // 实施引脚PWM设置
  //analogWrite(LED_WIFI, ledPwmVal);
  LED_WIFI.analog(ledPwmVal);

  server.sendHeader("Location", "/");
  server.send(303);
}

// 处理用户浏览器的HTTP访问
void handleUserRequest() {

  // 获取用户请求资源(Request Resource）
  String reqResource = server.uri();
  Serial.print("reqResource: ");
  Serial.println(reqResource);

  // 通过handleFileRead函数处处理用户请求资源
  bool fileReadOK = handleFileRead(reqResource);

  // 如果在SPIFFS无法找到用户访问的资源，则回复404 (Not Found)
  if (!fileReadOK) {
    server.send(404, "text/plain", "404 Not Found");
  }
}

bool handleFileRead(String resource) {            //处理浏览器HTTP访问

  if (resource.endsWith("/")) {                   // 如果访问地址以"/"为结尾
    resource = "/index.html";                     // 则将访问地址修改为/index.html便于SPIFFS访问
  }

  String contentType = getContentType(resource);  // 获取文件类型

  if (SPIFFS.exists(resource)) {                     // 如果访问的文件可以在SPIFFS中找到
    File file = SPIFFS.open(resource, "r");          // 则尝试打开该文件
    server.streamFile(file, contentType);// 并且将该文件返回给浏览器
    file.close();                                // 并且关闭文件
    return true;                                 // 返回true
  }
  return false;                                  // 如果文件未找到，则返回false
}

// 获取文件类型
String getContentType(String filename) {
  if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}
