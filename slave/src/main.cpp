#include <Arduino.h>
#include <FreeRTOS.h>

#include <WiFi.h>
// #include <WiFiManager.h>

#include <PubSubClient.h>
#include <AliyunIoTSDK.h>

#include <ArduinoJson.h>
#include <analogWrite.h>
#include <SoftwareSerial.h>

/**
 * 引脚定义
*/
#define ledWifiPin 15 // WIFI连接指示灯
#define ledMqttPin 2  // 云端连接指示灯

#define ledPin 5                // 测试用灯光
#define pwmPin 32               // white 电机PWM信号
#define ENPin 25                // yellow 电机制动信号
#define FRPin 26                // green 电机正反转信号
#define FGAPin 35               // 电机速度反馈信号1
#define FGBPin 34               // 电机速度反馈信号2
SoftwareSerial ASerial(18, 19); // 前激光测距传感器
SoftwareSerial BSerial(22, 23); // 后激光测距传感器
#define StopPin 13              // 到位光电传感器
// 接近开关
#define ENKey 0 //解除使能开关

// 连接WIFI账号和密码
#define WIFI_SSID "Origincell"
#define WIFI_PASSWD "Origincell123"

// 设备的三元组信息
#define PRODUCT_KEY "a15DkNAnYul"
#define DEVICE_NAME "Moth001"
#define DEVICE_SECRET "155f22f3f7b3cea7a9c3d61b9e59b82b"
#define REGION_ID "cn-shanghai"

/**
 * 变量初始化
*/
int flag = 0;
boolean _up = false;
int mode = 0;
unsigned char Adata[11] = {0};
unsigned char Bdata[11] = {0};
bool light = 0;
bool FR = 1; // 默认前进  // 正转
bool EN = 0; // 急停状态
uint16_t FG = 0;
uint16_t actualSpeed = 0;   // 实际速度
uint16_t expectSpeed = 100; // 期望速度
uint16_t MAXSpeed = 100;    // 最大速度

static WiFiClient CClient;

/**
 * Wifi连接模块
 * 有时候会链连接不上需要解决
*/
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

/**
 * 测试LED
*/
void lightCallback(JsonVariant p)
{
  light = p["light"];
  if (light != digitalRead(ledPin))
  {
    if (light == 1)
    {
      Serial.println("开灯");
    }
    else
    {
      Serial.println("关灯");
    }
    AliyunIoTSDK::send("light", light);
    digitalWrite(ledPin, light);
  }
}

/**
 * 速度反馈
 * 显示当期速度和期望速度
*/
void speedCallback(JsonVariant p)
{
  MAXSpeed = p["speed"];
  Serial.print("设置最大速度为: ");
  Serial.println(MAXSpeed);
  Serial.print("当前速度: ");
  Serial.println(actualSpeed);
  expectSpeed = MAXSpeed;
  AliyunIoTSDK::send("speed", actualSpeed);
}

// 正反转
void FRCallback(JsonVariant p)
{
  FR = p["move"];
  if (FR != digitalRead(FRPin))
  {
    if (FR == 1)
    {
      Serial.println("向前");
    }
    else
    {
      Serial.println("后退");
    }
    digitalWrite(FRPin, FR);
    AliyunIoTSDK::send("move", FR);
  }
}

/**
 * 制动
 * 有独立break引脚电机使用
*/
void ENCallback(JsonVariant p)
{
  EN = p["bbreak"];
  if (EN != digitalRead(ENPin))
  {
    if (EN == 1)
    {
      Serial.println("正常状态");
    }
    else
    {
      Serial.println("制动状态");
      actualSpeed = 0;
      expectSpeed = 0;
    }
    digitalWrite(ENPin, EN);
    AliyunIoTSDK::send("bbreak", EN);
    AliyunIoTSDK::send("speed", actualSpeed);
  }
}

// /**
//  * 制动
//  * 将pwm脚输出设置为0实现
// */
// void ENCallback(JsonVariant p)
// {
//   EN = p["bbreak"];
//   if (EN == 1)
//   {
//     Serial.println("正常状态");
//   }
//   else
//   {
//     Serial.println("制动状态");
//   }
//   AliyunIoTSDK::send("bbreak", EN);
//   AliyunIoTSDK::send("speed", 0);
// }

void toPark(int8_t Pin, bool state)
{
  //下面的程序放在loop()里
  if (digitalRead(Pin) == 1)
    flag = 1;
  if ((digitalRead(Pin) == 0) && (flag == 1))
  {
    _up = true;
    flag = 0;
  }

  while (_up)
  {
    //这里写希望检测到上升沿后执行的程序
    if ((digitalRead(Pin) == 0) && (flag == 1))
    {
      mode = 0;
      EN = 0;
      actualSpeed = 0;
      digitalWrite(ENPin, EN);
      AliyunIoTSDK::send("bbreak", EN);
      AliyunIoTSDK::send("speed", actualSpeed);
      _up = false;
    }
    else
    {
      actualSpeed = 20;
      AliyunIoTSDK::loop();
      analogWrite(pwmPin, map(actualSpeed, 0, 100, 0, 1000), 1000);
    }
    if (digitalRead(Pin) == 1)
    {
      flag = 1;
    }
  }
}

/*******************************************
 * 配置
 *******************************************/
void setup()
{
  Serial.begin(115200);
  ASerial.begin(9600);
  BSerial.begin(9600);
  Serial.println();
  Serial.println();
  Serial.println("///////////START///////////");
  delay(20);

  // pinMode(ledWifiPin, OUTPUT);
  pinMode(ledMqttPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(pwmPin, OUTPUT);
  pinMode(FRPin, OUTPUT);
  pinMode(ENPin, OUTPUT);
  digitalWrite(FRPin, FR);
  digitalWrite(ENPin, EN);

  pinMode(FGAPin, INPUT);
  pinMode(ENKey, INPUT_PULLUP);
  pinMode(StopPin, INPUT_PULLUP);
  // attachInterrupt(digitalPinToInterrupt(StopPin), toPark, FALLING);

  // 初始化 wifi
  wifiInit(WIFI_SSID, WIFI_PASSWD);
  Serial.println();

  // 初始化 iot，需传入 wifi 的 client，和设备产品信息
  AliyunIoTSDK::begin(CClient, PRODUCT_KEY, DEVICE_NAME, DEVICE_SECRET, REGION_ID);
  digitalWrite(ledMqttPin, 1);
  Serial.println();

  // char buff1[5] = {0xFA, 0x04, 0x09, 0x05, 0xF4}; // 设置量程 5m
  // ASerial.print(buff1);
  // char buff2[5] = {0xFA, 0x04, 0x0C, 0x02, 0xF4}; // 设定分辨率 .1mm
  // ASerial.print(buff2);
  char buff4[5] = {0xFA, 0x04, 0x05, 0x00, 0xFD}; // 时间间隔 1s
  ASerial.print(buff4);
  char buff3[5] = {0xFA, 0x04, 0x0D, 0x01, 0xF4}; // 设定上电就测
  ASerial.print(buff3);
  char buff0[4] = {0x80, 0x06, 0x03, 0x77}; // 连续测量
  ASerial.print(buff0);

  // attachInterrupt(speedPin, ccount, FALLING);
  // 绑定一个设备属性回调，当远程修改此属性，会触发 powerCallback
  // PowerSwitch 是在设备产品中定义的物联网模型的 id
  AliyunIoTSDK::bindData("light", lightCallback);
  AliyunIoTSDK::bindData("speed", speedCallback);
  AliyunIoTSDK::bindData("move", FRCallback);
  AliyunIoTSDK::bindData("bbreak", ENCallback);

  // 发送当前初始化状态
  AliyunIoTSDK::send("light", digitalRead(ledPin));
  AliyunIoTSDK::send("speed", actualSpeed);
  AliyunIoTSDK::send("move", digitalRead(FRPin));
  AliyunIoTSDK::send("bbreak", digitalRead(ENPin));
}

/*******************************************
 * 循环
 *******************************************/

void loop()
{

  AliyunIoTSDK::loop();
  analogWrite(pwmPin, map(actualSpeed, 0, 100, 0, 1000), 1000);
  // Serial.println(digitalRead(StopPin));
  if (digitalRead(ENPin) == 1)
  {
    toPark(StopPin, 0); //1为上升沿0为下降沿
  }

  if (digitalRead(ENKey) != 1)
  {
    // 模式切换
    delay(500);
    mode++;
    delay(500);
    if (mode > 2)
    {
      mode = 0;
    }
    if (1 == mode)
    {
      Serial.println("--模式1--");
      EN = 1;
      digitalWrite(ENPin, EN);
      AliyunIoTSDK::send("bbreak", EN);
      FR = 1;
      digitalWrite(FRPin, FR);
      AliyunIoTSDK::send("move", FR);
    }
    else if (2 == mode)
    {
      Serial.println("--模式2--");
      EN = 1;
      digitalWrite(ENPin, EN);
      AliyunIoTSDK::send("bbreak", EN);
      FR = 0;
      digitalWrite(FRPin, FR);
      AliyunIoTSDK::send("move", FR);
    }
    else
    {
      Serial.println("--模式0--");
      EN = 0;
      actualSpeed = 0;
      digitalWrite(ENPin, EN);
      AliyunIoTSDK::send("bbreak", EN);
      AliyunIoTSDK::send("speed", actualSpeed);
    }
  }

  if (!CClient.connected())
  {
    digitalWrite(ledWifiPin, 0);
    digitalWrite(ledMqttPin, 0);
    actualSpeed = 0;
    AliyunIoTSDK::send("light", digitalRead(ledPin));
    AliyunIoTSDK::send("speed", actualSpeed);
    AliyunIoTSDK::send("move", digitalRead(FRPin));
    AliyunIoTSDK::send("bbreak", digitalRead(ENPin));
  }

  if (actualSpeed != expectSpeed)
  {
    if (1 == EN)
    {
      actualSpeed = expectSpeed;
      AliyunIoTSDK::send("speed", actualSpeed);
    }
    else
    {
      actualSpeed = 0;
    }
  }

  // 激光测距
  if (ASerial.available() > 0) // 判断串口是否有数据可读
  {

    delay(40);
    for (int i = 0; i < 11; i++)
    {
      Adata[i] = ASerial.read();
    }
    unsigned char Check = 0;
    for (int i = 0; i < 10; i++)
    {
      Check = Check + Adata[i];
    }
    Check = ~Check + 1;
    if (Adata[10] == Check)
    {
      float distance = 0;
      if (Adata[3] == 'E' && Adata[4] == 'R' && Adata[5] == 'R')
      {
        distance = 0;
      }
      else
      {
        distance = (Adata[3] - 0x30) * 100 + (Adata[4] - 0x30) * 10 + (Adata[5] - 0x30) * 1 + (Adata[7] - 0x30) * 0.1 + (Adata[8] - 0x30) * 0.01 + (Adata[9] - 0x30) * 0.001;
      }
      Serial.println(distance);
      if (distance <= 0.3)
      {
        expectSpeed = 0;
      }
      else if ((distance <= 1) & (distance > 0.3))
      {
        expectSpeed = MAXSpeed * 0.5;
      }
      else
      {
        expectSpeed = MAXSpeed;
      }
    }
    else
    {
      Serial.println("Invalid Data!");
    }
  }
}
