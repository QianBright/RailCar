#include <Arduino.h>

#include <WiFi.h>
#include <PubSubClient.h>

#include <ArduinoJson.h>
#include <analogWrite.h>

// 引脚设置
#define KEY_IO0 0
#define ledPin 12
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

#define CLIENT_ID "123|securemode=3,signmethod=hmacsha1,timestamp=789|"
#define MQTT_PASSWD "9389b234f5435b0f8b031d7f6403ddc73a73e758"

// 线上环境域名和端口号，不需要改
#define MQTT_SERVER PRODUCT_KEY ".iot-as-mqtt." REGION_ID ".aliyuncs.com"
#define MQTT_PORT 1883
#define MQTT_USRNAME DEVICE_NAME "&" PRODUCT_KEY

#define ALINK_TOPIC_PROP_POST "/sys/" PRODUCT_KEY "/" DEVICE_NAME "/thing/event/property/post"
#define ALINK_TOPIC_PROP_SET "/sys/" PRODUCT_KEY "/" DEVICE_NAME "/thing/event/property/set"

#define ALINK_BODY_FORMAT "{\"method\":\"thing.service.property.post\",\"id\":\"123\",\"params\":{},\"version\":\"1.0.0\"}"

unsigned long lastMs = 0;
// 为会接收到的变量设置全局变量并赋初始值
bool light = 0;
bool move = 1;       // 默认前进  // 正转
bool bbreak = 1;     // 急停状态
int actualSpeed = 0; // 实际速度
int expectSpeed = 0; // 期望速度

WiFiClient CClient;
PubSubClient client(CClient);

// 制动
void bBreak()
{
  actualSpeed = 0;
  expectSpeed = 0;
}

// 加减速度
int speedchange(uint8_t pin, uint32_t START, uint32_t END, uint32_t TIME)
{
  // 将变量val数值从0~100区间映射到1000~0区间
  START = map(START, 0, 100, 1000, 0);
  END = map(END, 0, 100, 1000, 0);
  int delayTime = floor(TIME / 1000);

  while (START != END)
  {
    analogWrite(pwmPin, START, 1000);
    delay(delayTime);
    if (START < END)
    {
      START++;
    }
    else
    {
      START--;
    }
  }
  analogWrite(pwmPin, START, 1000);

  return map(START, 1000, 0, 0, 100);
}

// Wifi连接模块
void wifiInit()
{
  WiFi.mode(WIFI_AP_STA);
  // WiFi.softAP("eDray", "12345678");
  Serial.print("WiFi连接中..");
  WiFi.begin(WIFI_SSID, WIFI_PASSWD);
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
}

// 检查与服务器的连接状态 // 断开重连
void mqttConnect()
{
  while (!client.connected())
  {
    Serial.println("MQTT服务器连接中...");
    if (client.connect(CLIENT_ID, MQTT_USRNAME, MQTT_PASSWD))
    {
      Serial.println("MQTT连接成功!");
      // 连接成功时订阅主题
      client.subscribe(ALINK_TOPIC_PROP_SET); // 订阅
    }
    else
    {
      Serial.print("MQTT Connect err:");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

// PUSH
void mqttIntervalPost(int LIGHT, int MOVE, int BBREAK, int SPEED)
{
  DynamicJsonDocument PUSHdoc(512);
  deserializeJson(PUSHdoc, ALINK_BODY_FORMAT); //从ALINK_BODY_FORMAT解码成DynamicJsonDocument
  JsonObject root = PUSHdoc.as<JsonObject>();  //从doc对象转换成的JsonObject类型对象
  JsonObject params = root["params"];
  //需要上传的变量/值在此添加
  params["light"] = LIGHT;
  params["move"] = MOVE;
  params["bbreak"] = BBREAK;
  params["speed"] = SPEED;

  Serial.println("-----------PUSH MESSAGE------------");
  Serial.println("消息内容: ");
  serializeJsonPretty(PUSHdoc, Serial); // 串口美化打印json
  Serial.println();

  Serial.printf("JsonDocument已使用内存 %d Byte\n", PUSHdoc.memoryUsage());

  char jsonBuffer[256];
  size_t n = serializeJson(PUSHdoc, jsonBuffer);
  client.publish(ALINK_TOPIC_PROP_POST, jsonBuffer, n);
  // PUSHdoc.clear();
  Serial.println("-----------PUSH END------------");
}

// RECV
// 首先打印主题名称, 然后打印收到的消息的每个字节
void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.println("-----------RECV MESSAGE------------");
  Serial.print("Topic: ");
  Serial.println(topic);

  DynamicJsonDocument RECVdoc(512);
  deserializeJson(RECVdoc, payload, length);
  // 显示接收到的信息
  Serial.println("消息内容: ");
  serializeJsonPretty(RECVdoc, Serial); // 串口美化打印json
  Serial.println();
  Serial.printf("JsonDocument已使用内存 %d Byte\n", RECVdoc.memoryUsage());

  JsonObject root = RECVdoc.as<JsonObject>(); //从doc对象转换成的JsonObject类型对象
  JsonObject params = root["params"];
  // 接受到的变量/值在此添加
  if (!params["light"].isNull())
  {
    light = params.getMember("light");
  }

  if (!params["move"].isNull())
  {
    move = params.getMember("move");
    actualSpeed = speedchange(pwmPin, actualSpeed, 0, 5000);
  }
  if (!params["bbreak"].isNull())
  {
    bbreak = params.getMember("bbreak");
    bBreak();
  }
  if (!params["speed"].isNull())
  {
    expectSpeed = params.getMember("speed");
  }

  // RECVdoc.clear();
  Serial.println("-----------RECV END------------");
  mqttIntervalPost(digitalRead(ledPin), move, bbreak, actualSpeed);
}

void setup()
{
  Serial.begin(115200);
  pinMode(KEY_IO0, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);

  pinMode(breakPin, OUTPUT);
  pinMode(pwmPin, OUTPUT);
  pinMode(reversalPin, OUTPUT);

  Serial.println();
  Serial.println();
  Serial.println("///////////Start///////////");

  wifiInit(); // 连接WIFI

  // 连接MQTT服务器
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback); //接受消息
  mqttConnect();
}

void loop()
{
  // 重连
  if (!client.connected())
  {
    mqttConnect();
  }
  // 5秒上传一次,长传当前状态
  if (millis() - lastMs >= 5000)
  {
    lastMs = millis();
    mqttIntervalPost(digitalRead(ledPin), move, bbreak, actualSpeed); // 发布
    Serial.println(light);
    Serial.println(move);
    Serial.println(bbreak);
    Serial.println(expectSpeed);
  }
  //led_wifi
  digitalWrite(ledPin, light);
  digitalWrite(reversalPin, move);
  digitalWrite(breakPin, bbreak);

  // 实际速度按照线性接近期望速度，最终实际速度=期望速度
  actualSpeed = speedchange(pwmPin, actualSpeed, expectSpeed, 10000);
  client.loop();
}
