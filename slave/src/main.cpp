#include <Arduino.h>

#include <WiFi.h>
// #include <WiFiManager.h>

#include <PubSubClient.h>
#include <AliyunIoTSDK.h>

#include <ArduinoJson.h>
#include <analogWrite.h>

// 引脚设置
#define KEY_IO0 0
#define ledPin 12
#define ledWifiPin 32
#define ledMqttPin 33
#define breakPin 18    // yellow
#define pwmPin 19      // white
#define reversalPin 21 // green

// 连接WIFI账号和密码
#define WIFI_SSID "origincell-auto"
#define WIFI_PASSWD "origincell"

// 设备的三元组信息
#define PRODUCT_KEY "a15DkNAnYul"
#define DEVICE_NAME "Moth001"
#define DEVICE_SECRET "155f22f3f7b3cea7a9c3d61b9e59b82b"
#define REGION_ID "cn-shanghai"

#define ALINK_TOPIC_PROP_POST "/sys/" PRODUCT_KEY "/" DEVICE_NAME "/thing/event/property/post"
#define ALINK_TOPIC_PROP_SET "/sys/" PRODUCT_KEY "/" DEVICE_NAME "/thing/event/property/set"

#define ALINK_BODY_FORMAT "{\"method\":\"thing.service.property.post\",\"id\":\"123\",\"params\":{},\"version\":\"1.0.0\"}"

unsigned long lastMs = 0;
// 为会接收到的变量设置全局变量并赋初始值
int light = 0;

static WiFiClient CClient;

// Wifi连接模块
void wifiInit(const char *SSID, const char *PASSWORD)
{
  WiFi.mode(WIFI_AP_STA);
  // WiFi.softAP("eDray", "12345678");
  Serial.print("WiFi连接中..");
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi连接成功!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC 地址: ");
  Serial.println(WiFi.macAddress());
  digitalWrite(ledWifiPin, 1);
}

// 电源属性修改的回调函数
void lightCallback(JsonVariant p)
{
  int light = p["light"];
  if (light == 1)
  {
    // 启动设备
    Serial.println("开灯");
  }
  else
  {
    Serial.println("关灯");
  }

  digitalWrite(ledPin, light);
}

void speedCallback(JsonVariant p)
{
  int speed = p["speed"];
  Serial.println(speed);
}

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println("///////////Start///////////");
  delay(20);

  pinMode(KEY_IO0, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  pinMode(ledWifiPin, OUTPUT);
  pinMode(ledMqttPin, OUTPUT);

  pinMode(breakPin, OUTPUT);
  pinMode(pwmPin, OUTPUT);
  pinMode(reversalPin, OUTPUT);

  // 初始化 wifi
  wifiInit(WIFI_SSID, WIFI_PASSWD);

  // 初始化 iot，需传入 wifi 的 client，和设备产品信息
  AliyunIoTSDK::begin(CClient, PRODUCT_KEY, DEVICE_NAME, DEVICE_SECRET, REGION_ID);
  digitalWrite(ledMqttPin, 1);

  // 绑定一个设备属性回调，当远程修改此属性，会触发 powerCallback
  // PowerSwitch 是在设备产品中定义的物联网模型的 id
  AliyunIoTSDK::bindData("light", lightCallback);
  AliyunIoTSDK::bindData("speed", speedCallback);

  // 发送一个数据到云平台，LightLuminance 是在设备产品中定义的物联网模型的 id
  AliyunIoTSDK::send("light", digitalRead(KEY_IO0));
  AliyunIoTSDK::send("speed", 0);
}

void loop()
{
  AliyunIoTSDK::loop();
  if (!CClient.connected())
  {
    digitalWrite(ledWifiPin, 0);
    digitalWrite(ledMqttPin, 0);
  }
}
