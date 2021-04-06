#include <Arduino.h>
#include <FreeRTOS.h>

#include <WiFi.h>
// #include <WiFiManager.h>

#include <PubSubClient.h>
#include <AliyunIoTSDK.h>

#include <ArduinoJson.h>
#include <analogWrite.h>

/**
 * 输出引脚
*/
#define ledWifiPin 32
#define ledMqttPin 33

#define ledPin 12
// #define breakPin 18    // yellow
#define pwmPin 19      // white
#define reversalPin 21 // green

/**
 * 输入引脚
 * 仅两个脚支持模拟量输入
*/
#define KEY_IO0 0
#define speedPin 5

// 连接WIFI账号和密码
#define WIFI_SSID "Origincell"
#define WIFI_PASSWD "Origincell123"

// 设备的三元组信息
#define PRODUCT_KEY "a15DkNAnYul"
#define DEVICE_NAME "Moth001"
#define DEVICE_SECRET "155f22f3f7b3cea7a9c3d61b9e59b82b"
#define REGION_ID "cn-shanghai"

// 为会接收到的变量设置全局变量并赋初始值
bool light = 0;
bool move = 1;              // 默认前进  // 正转
bool bbreak = 1;            // 急停状态
uint16_t actualSpeed = 0;   // 实际速度
uint16_t registerSpeed = 0; //正反转切换时暂存速度
uint16_t expectSpeed = 0;   // 期望速度

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
 * 加减速
 * 由于必须使用delay造成加减速期间无法收到命令，故舍弃
*/
// int speedchange(uint8_t pin, uint32_t START, uint32_t END, uint32_t TIME)
// {
//   // 将变量val数值从0~100区间映射到1000~0区间
//   START = map(START, 0, 100, 1000, 0);
//   END = map(END, 0, 100, 1000, 0);
//   int delayTime = floor(TIME / 1000);

//   while (START != END)
//   {
//     int a = map(START, 1000, 0, 0, 100);
//     analogWrite(pwmPin, START, 1000);
//     delayMicroseconds(uint32_t(delayTime * 1000));
//     // AliyunIoTSDK::loop(); // 希望采用多任务来解决它
//     AliyunIoTSDK::send("speed", a);
//     if (START < END)
//     {
//       START++;
//     }
//     else
//     {
//       START--;
//     }
//   }
//   analogWrite(pwmPin, START, 1000);
//   return map(START, 1000, 0, 0, 100);
// }

/**
 * 灯光
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
  expectSpeed = p["speed"];
  Serial.print("速度需要达到: ");
  Serial.println(expectSpeed);
  Serial.print("当前速度: ");
  Serial.println(actualSpeed);
}

// move
void moveCallback(JsonVariant p)
{
  move = p["move"];
  if (move != digitalRead(reversalPin))
  {
    if (move == 1)
    {
      Serial.println("向前");
    }
    else
    {
      Serial.println("后退");
    }
    registerSpeed = expectSpeed;
    expectSpeed = 0;
    // AliyunIoTSDK::send("speed", actualSpeed);
  }
}

/**
 * 制动
 * 有独立break引脚电机使用
*/
// void bbreakCallback(JsonVariant p)
// {
//   bbreak = p["bbreak"];
//   if (bbreak != digitalRead(breakPin))
//   {
//     if (bbreak == 1)
//     {
//       Serial.println("正常状态");
//     }
//     else
//     {
//       Serial.println("制动状态");
//     }
//     digitalWrite(breakPin, bbreak);
//     AliyunIoTSDK::send("bbreak", digitalRead(breakPin));
//     if (bbreak == 0)
//     {
//       actualSpeed = 0;
//       expectSpeed = 0;
//     }
//     AliyunIoTSDK::send("speed", actualSpeed);
//   }
// }

/**
 * 制动
 * 将pwm脚输出设置为0实现
*/
void bbreakCallback(JsonVariant p)
{
  bbreak = p["bbreak"];
  if (bbreak == 1)
  {
    Serial.println("正常状态");
  }
  else
  {
    Serial.println("制动状态");
  }
  AliyunIoTSDK::send("bbreak", bbreak);
  AliyunIoTSDK::send("speed", 0);
}

// 多任务模块
// void TaskMove(void *pvParameters)
// {
//   Serial.print("TaskLoop running on core: ");
//   Serial.println(xPortGetCoreID());
//   while (1)
//   {
//     actualSpeed = speedchange(pwmPin, actualSpeed, expectSpeed, 10000);
//   }
// }

/*******************************************
 * 配置
 *******************************************/
void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println("///////////START///////////");
  delay(20);

  pinMode(ledWifiPin, OUTPUT);
  pinMode(ledMqttPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(pwmPin, OUTPUT);
  pinMode(reversalPin, OUTPUT);
  // pinMode(breakPin, OUTPUT);
  digitalWrite(pwmPin, 0);
  digitalWrite(reversalPin, move);

  pinMode(KEY_IO0, INPUT_PULLUP);
  pinMode(speedPin, INPUT);

  // 初始化 wifi
  wifiInit(WIFI_SSID, WIFI_PASSWD);
  Serial.println();

  // 初始化 iot，需传入 wifi 的 client，和设备产品信息
  AliyunIoTSDK::begin(CClient, PRODUCT_KEY, DEVICE_NAME, DEVICE_SECRET, REGION_ID);
  digitalWrite(ledMqttPin, 1);
  Serial.println();

  // 绑定一个设备属性回调，当远程修改此属性，会触发 powerCallback
  // PowerSwitch 是在设备产品中定义的物联网模型的 id
  AliyunIoTSDK::bindData("light", lightCallback);
  AliyunIoTSDK::bindData("speed", speedCallback);
  AliyunIoTSDK::bindData("move", moveCallback);
  AliyunIoTSDK::bindData("bbreak", bbreakCallback);

  // 发送当前初始化状态
  AliyunIoTSDK::send("light", digitalRead(ledPin));
  AliyunIoTSDK::send("speed", actualSpeed);
  AliyunIoTSDK::send("move", digitalRead(reversalPin));
  AliyunIoTSDK::send("bbreak", bbreak);
}

/*******************************************
 * 循环
 *******************************************/
void loop()
{
  AliyunIoTSDK::loop();
  analogWrite(pwmPin, map(actualSpeed, 0, 100, 0, 1000), 1000);

  Serial.println(pulseIn(speedPin, HIGH, 5000UL));

  if (!CClient.connected())
  {
    digitalWrite(ledWifiPin, 0);
    digitalWrite(ledMqttPin, 0);
  }
  if (bbreak == 0)
  {
    actualSpeed = 0;
    expectSpeed = 0;
    move = 1;
  }
  if (actualSpeed != expectSpeed)
  {
    if (actualSpeed < expectSpeed)
    {
      actualSpeed++;
    }
    else
    {
      actualSpeed--;
    }
    // analogWrite(pwmPin, map(actualSpeed, 0, 100, 0, 1000), 1000);
    AliyunIoTSDK::send("speed", actualSpeed);
    delayMicroseconds(5000);
  }

  if (actualSpeed == 0 && move != digitalRead(reversalPin))
  {
    expectSpeed = registerSpeed;
    digitalWrite(reversalPin, move);
    AliyunIoTSDK::send("move", move);
  }
}
