#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// 引脚设置
#define KEY_IO0 0
#define LED 12

// 连接WIFI账号和密码
#define WIFI_SSID         "origincell-auto"
#define WIFI_PASSWD       "origincell"


// 设备的三元组信息
#define PRODUCT_KEY       "a15DkNAnYul"
#define DEVICE_NAME       "Moth001"
#define DEVICE_SECRET     "155f22f3f7b3cea7a9c3d61b9e59b82b"
#define REGION_ID         "cn-shanghai"

#define CLIENT_ID         "123|securemode=3,signmethod=hmacsha1,timestamp=789|"
#define MQTT_PASSWD       "9389b234f5435b0f8b031d7f6403ddc73a73e758"

// 线上环境域名和端口号，不需要改
#define MQTT_SERVER       PRODUCT_KEY ".iot-as-mqtt." REGION_ID ".aliyuncs.com"
#define MQTT_PORT         1883
#define MQTT_USRNAME      DEVICE_NAME "&" PRODUCT_KEY

#define ALINK_TOPIC_PROP_POST     "/sys/" PRODUCT_KEY "/" DEVICE_NAME "/thing/event/property/post"
#define ALINK_TOPIC_PROP_SET     "/sys/" PRODUCT_KEY "/" DEVICE_NAME "/thing/event/property/set"

#define ALINK_BODY_FORMAT         "{\"method\":\"thing.service.property.post\",\"id\":\"123\",\"params\":{},\"version\":\"1.0.0\"}"

bool light = 0;
unsigned long lastMs = 0;

WiFiClient CClient;
PubSubClient  client(CClient);

// Wifi连接模块
void wifiInit() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("eDray", "12345678");
  Serial.print("WiFi连接中..");
  WiFi.begin(WIFI_SSID, WIFI_PASSWD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi连接成功!");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}



// 检查与服务器的连接状态 // 断开重连
void mqttConnect() {
  while (!client.connected()) {
    Serial.println("MQTT服务器连接中...");
    if (client.connect(CLIENT_ID, MQTT_USRNAME, MQTT_PASSWD)) {
      Serial.println("MQTT连接成功!");
      // 连接成功时订阅主题
      client.subscribe(ALINK_TOPIC_PROP_SET); // 订阅
    } else {
      Serial.print("MQTT Connect err:");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

// PUSH
void mqttIntervalPost() {
  DynamicJsonDocument PUSHdoc(512);
  deserializeJson(PUSHdoc, ALINK_BODY_FORMAT);//从ALINK_BODY_FORMAT解码成DynamicJsonDocument
  JsonObject root = PUSHdoc.as<JsonObject>(); //从doc对象转换成的JsonObject类型对象
  JsonObject params = root["params"];
  params["light"] = digitalRead(KEY_IO0);

  Serial.println("消息内容: ");
  serializeJsonPretty(PUSHdoc, Serial);// 串口美化打印json
  Serial.println();

  Serial.printf("JsonDocument已使用内存%d字节\n", PUSHdoc.memoryUsage());

  char jsonBuffer[256];
  size_t n = serializeJson(PUSHdoc, jsonBuffer);
  client.publish(ALINK_TOPIC_PROP_POST, jsonBuffer, n);

  Serial.println();
  Serial.println("-----------PUSH END------------");
  //  PUSHdoc.clear();
}

// RECV
//首先打印主题名称, 然后打印收到的消息的每个字节
void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Topic: ");
  Serial.println(topic);
  DynamicJsonDocument RECVdoc(512);
  deserializeJson(RECVdoc, payload, length);
  //  JsonObject root = RECVdoc.as<JsonObject>(); //从doc对象转换成的JsonObject类型对象
  //  JsonObject params = root["params"];
  Serial.println("消息内容: ");
  serializeJsonPretty(RECVdoc, Serial);// 串口美化打印json
  Serial.println();
  Serial.printf("JsonDocument已使用内存%d字节\n", RECVdoc.memoryUsage());

  JsonObject root = RECVdoc.as<JsonObject>(); //从doc对象转换成的JsonObject类型对象
  JsonObject params = root["params"];
  light = params["light"];
  //  RECVdoc.clear();
  Serial.println("-----------RECV END------------");
}

void setup() {
  Serial.begin(115200);
  pinMode(KEY_IO0,  INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  Serial.println();
  Serial.println();
  Serial.println("///////////Start///////////");

  wifiInit(); // 连接WIFI

  // 连接MQTT服务器
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback); //接受消息
  mqttConnect();
  //  mqttIntervalPost(); // 发布

}

void loop() {
  if (!client.connected()) {
    mqttConnect();
  }
  // 5秒上传一次
  if (millis() - lastMs >= 5000) {
    lastMs = millis();
    mqttIntervalPost(); // 发布
  }
  client.loop();
  if (light == 1) {
    digitalWrite(LED, HIGH);
  } else {
    digitalWrite(LED, LOW);
  }
}
