#include "AliyunIoTSDK.h"
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SHA256.h>

#define CHECK_INTERVAL 5000 // 5s
#define MESSAGE_BUFFER_SIZE 10

static const char *deviceName = NULL;
static const char *productKey = NULL;
static const char *deviceSecret = NULL;
static const char *region = NULL;
static const char *CLIENT_ID = NULL;

struct DeviceProperty
{
    String key;
    String value;
};

DeviceProperty PropertyMessageBuffer[MESSAGE_BUFFER_SIZE];

#define MQTT_PORT 1883

#define SHA256HMAC_SIZE 32
#define DATA_CALLBACK_SIZE 20

#define ALINK_BODY_FORMAT "{\"id\":\"%s\",\"params\":%s,\"version\":\"1.0\",\"method\":\"thing.event.property.post\"}"
#define ALINK_EVENT_BODY_FORMAT "{\"id\":\"%s\",\"params\": %s,\"version\": \"1.0\",\"method\": \"thing.event.%s.post\"}"

static unsigned long lastMs = 0;
static int retry_count = 0;

static PubSubClient *client = NULL;

char AliyunIoTSDK::clientId[256] = "";
char AliyunIoTSDK::mqttUsername[100] = "";
char AliyunIoTSDK::mqttPwd[256] = "";
char AliyunIoTSDK::domain[150] = "";

char AliyunIoTSDK::ALINK_TOPIC_PROP_POST[150] = "";
char AliyunIoTSDK::ALINK_TOPIC_PROP_SET[150] = "";
char AliyunIoTSDK::ALINK_TOPIC_EVENT[150] = "";

// 观察PropertyMessageBuffer里面的情况
void showBuffer()
{
    Serial.println("---------------------------------------------------");
    Serial.print(" | ");
    Serial.print(PropertyMessageBuffer[0].key + ":" + PropertyMessageBuffer[0].value);
    Serial.print(" | ");
    Serial.print(PropertyMessageBuffer[1].key + ":" + PropertyMessageBuffer[1].value);
    Serial.print(" | ");
    Serial.print(PropertyMessageBuffer[2].key + ":" + PropertyMessageBuffer[2].value);
    Serial.print(" | ");
    Serial.print(PropertyMessageBuffer[3].key + ":" + PropertyMessageBuffer[3].value);
    Serial.print(" | ");
    Serial.print(PropertyMessageBuffer[4].key + ":" + PropertyMessageBuffer[4].value);
    Serial.println(" | ");
    Serial.println("---------------------------------------------------");
    Serial.print(" | ");
    Serial.print(PropertyMessageBuffer[5].key + ":" + PropertyMessageBuffer[5].value);
    Serial.print(" | ");
    Serial.print(PropertyMessageBuffer[6].key + ":" + PropertyMessageBuffer[6].value);
    Serial.print(" | ");
    Serial.print(PropertyMessageBuffer[7].key + ":" + PropertyMessageBuffer[7].value);
    Serial.print(" | ");
    Serial.print(PropertyMessageBuffer[8].key + ":" + PropertyMessageBuffer[8].value);
    Serial.print(" | ");
    Serial.print(PropertyMessageBuffer[9].key + ":" + PropertyMessageBuffer[9].value);
    Serial.println(" | ");
    Serial.println("---------------------------------------------------");
}

// 观察poniter_array里面的情况
void showPoniteArray()
{
    Serial.println("---------------------------------------------------");
    Serial.print(" | ");
    Serial.print(poniter_array[0].key);
    Serial.print(" | ");
    Serial.print(poniter_array[1].key);
    Serial.print(" | ");
    Serial.print(poniter_array[2].key);
    Serial.print(" | ");
    Serial.print(poniter_array[3].key);
    Serial.print(" | ");
    Serial.print(poniter_array[4].key);
    Serial.println(" | ");
    Serial.println("---------------------------------------------------");
    Serial.print(" | ");
    Serial.print(poniter_array[5].key);
    Serial.print(" | ");
    Serial.print(poniter_array[6].key);
    Serial.print(" | ");
    Serial.print(poniter_array[7].key);
    Serial.print(" | ");
    Serial.print(poniter_array[8].key);
    Serial.print(" | ");
    Serial.print(poniter_array[9].key);
    Serial.println(" | ");
    Serial.println("---------------------------------------------------");
}

static String hmac256(const String &signcontent, const String &ds)
{
    byte hashCode[SHA256HMAC_SIZE];
    SHA256 sha256;

    const char *key = ds.c_str();
    size_t keySize = ds.length();

    sha256.resetHMAC(key, keySize);
    sha256.update((const byte *)signcontent.c_str(), signcontent.length());
    sha256.finalizeHMAC(key, keySize, hashCode, sizeof(hashCode));

    String sign = "";
    for (byte i = 0; i < SHA256HMAC_SIZE; ++i)
    {
        sign += "0123456789ABCDEF"[hashCode[i] >> 4];
        sign += "0123456789ABCDEF"[hashCode[i] & 0xf];
    }

    return sign;
}

// MQTT服务器连接
int AliyunIoTSDK::mqttCheckConnect()
{
    if ((!client->connected()) && (client != NULL))
    {
        client->disconnect();
        Serial.println("Connecting to MQTT Server ...");
        if (client->connect(clientId, mqttUsername, mqttPwd))
        {
            Serial.println("MQTT Connected!");
            Serial.println();
            return 1;
        }
        else
        {
            Serial.print("MQTT Connect err:");
            Serial.println(client->state());
            delayMicroseconds(2000000);
            return 0;
        }
    }
    return !((!client->connected()) && (client != NULL));
}

static void parmPass(JsonVariant parm)
{
    // const char *method = parm["method"];
    for (int i = 0; i < DATA_CALLBACK_SIZE; i++)
    {
        if (poniter_array[i].key)
        {
            // 接受root里的参数
            if (1 == parm.containsKey(poniter_array[i].key))
            {
                poniter_array[i].fp(parm);
                // serializeJsonPretty(parm, Serial);
                // Serial.println();
            }

            //接受params里的参数
            if (1 == parm["params"].containsKey(poniter_array[i].key))
            {
                poniter_array[i].fp(parm["params"]);
            }
        }
    }
}

// 所有云服务的回调都会首先进入这里，例如属性下发
static void callback(char *topic, byte *payload, unsigned int length)
{
    Serial.println();
    // Serial.println((char *)payload);
    if (strstr(topic, AliyunIoTSDK::ALINK_TOPIC_PROP_SET))
    {
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload, length); //反序列化JSON数据
        if (!error)                                                         //检查反序列化是否成功
        {
            Serial.println("-----------RECV MESSAGE------------");
            Serial.print("Topic: ");
            Serial.println(topic);
            Serial.println();
            parmPass(doc.as<JsonVariant>()); //将参数传递后打印输出

            // Serial.println("//////doc: //////");
            // serializeJsonPretty(doc, Serial);
            // Serial.println();

            Serial.println("-----------RECV END------------");
        }
        else
        {
            Serial.println("RECV error!");
        }
    }
    Serial.println();
}

void AliyunIoTSDK::begin(Client &espClient,
                         const char *_productKey,
                         const char *_deviceName,
                         const char *_deviceSecret,
                         const char *_region)
{

    client = new PubSubClient(espClient);
    client->setBufferSize(1024);
    client->setKeepAlive(60);
    productKey = _productKey;
    deviceName = _deviceName;
    deviceSecret = _deviceSecret;
    CLIENT_ID = _deviceName;
    region = _region;
    long times = millis();
    String timestamp = String(times);

    sprintf(clientId, "%s|securemode=3,signmethod=hmacsha256,timestamp=%s|", deviceName, timestamp.c_str());

    String signcontent = "clientId";
    signcontent += deviceName;
    signcontent += "deviceName";
    signcontent += deviceName;
    signcontent += "productKey";
    signcontent += productKey;
    signcontent += "timestamp";
    signcontent += timestamp;

    String pwd = hmac256(signcontent, deviceSecret);

    strcpy(mqttPwd, pwd.c_str());

    sprintf(mqttUsername, "%s&%s", deviceName, productKey);
    sprintf(ALINK_TOPIC_PROP_POST, "/sys/%s/%s/thing/event/property/post", productKey, deviceName);
    sprintf(ALINK_TOPIC_PROP_SET, "/sys/%s/%s/thing/service/property/set", productKey, deviceName);
    sprintf(ALINK_TOPIC_EVENT, "/sys/%s/%s/thing/event", productKey, deviceName);

    sprintf(domain, "%s.iot-as-mqtt.%s.aliyuncs.com", productKey, region);
    client->setServer(domain, MQTT_PORT); /* 连接WiFi之后，连接MQTT服务器 */
    client->setCallback(callback);

    mqttCheckConnect();
}

void AliyunIoTSDK::loop()
{

    client->loop();
    if (mqttCheckConnect())
    {
        messageBufferCheck();
    }
}

// void AliyunIoTSDK::sendEvent(const char *eventId, const char *param)
// {
//     char topicKey[156];
//     sprintf(topicKey, "%s/%s/post", ALINK_TOPIC_EVENT, eventId);
//     char jsonBuf[1024];
//     sprintf(jsonBuf, ALINK_EVENT_BODY_FORMAT, CLIENT_ID, param, eventId);
//     Serial.println(jsonBuf);
//     boolean d = client->publish(topicKey, jsonBuf);
//     Serial.print("publish:0 成功:");
//     Serial.println(d);
// }
// void AliyunIoTSDK::sendEvent(const char *eventId)
// {
//     sendEvent(eventId, "{}");
// }

// unsigned long lastSendMS = 0;

// 发送 buffer 数据
void sendBuffer(int16_t bufferSize)
{
    int i;
    String buffer;
    for (i = 0; i < bufferSize; i++)
    {
        // if (PropertyMessageBuffer[i].key.length() > 0)
        // {
        buffer += "\"" + PropertyMessageBuffer[i].key + "\":" + PropertyMessageBuffer[i].value + ",";
        // 转存到buffer，存完清空
        PropertyMessageBuffer[i].key = "";
        PropertyMessageBuffer[i].value = "";
        // }
    }

    buffer = "{" + buffer.substring(0, buffer.length() - 1) + "}";
    // Serial.print("buffer: ");
    // Serial.println(buffer);
    AliyunIoTSDK::send(buffer.c_str());
}

// 检查是否有数据，如有则发送
void AliyunIoTSDK::messageBufferCheck()
{
    int bufferSize = 0;
    for (int i = 0; (i < MESSAGE_BUFFER_SIZE) & (PropertyMessageBuffer[i].key.length() != 0); i++)
    {
        bufferSize++;
    }
    if (bufferSize > 0)
    {
        Serial.print("bufferSize: ");
        Serial.println(bufferSize);
        showBuffer(); //看看发了些啥
        sendBuffer(bufferSize);
    }
}

// 添加 key 和 value 到 PropertyMessageBuffer 里
void addMessageToBuffer(char *key, String value)
{
    size_t i = 0;
    size_t breakMark = 0; //break标志位
    for (i = 0; (i < MESSAGE_BUFFER_SIZE) & (PropertyMessageBuffer[i].key.length() != 0); i++)
    {

        if (PropertyMessageBuffer[i].key == key)
        {
            if (PropertyMessageBuffer[i].value != value)
            {
                // 发送完buffer就是空了, 那么还是会继续发送
                sendBuffer(i + 1); // 只要值改变就去发送
                i = 0;
                PropertyMessageBuffer[i].value = value;
            }
            breakMark = 1;
            break;
        }
        if (breakMark == 1)
        {
            break;
        }
    }
    // Serial.print("i:");
    // Serial.println(i);
    if (i == MESSAGE_BUFFER_SIZE) //如果buffer装满了，则发送
    {
        Serial.println("PropertyMessageBuffer即将超出, 自动push");
        sendBuffer(i);
        i = 0;
    }
    // else if (i > MESSAGE_BUFFER_SIZE)
    // {
    //     Serial.println("[Warning!]超出PropertyMessageBuffer的范围了!");
    //     Serial.print("[Warning!]MESSAGE_BUFFER_SIZE = ");
    //     Serial.println(MESSAGE_BUFFER_SIZE);
    // }

    for (i = i; i < MESSAGE_BUFFER_SIZE; i++)
    {
        if (breakMark == 1)
        {
            break;
        }
        if (PropertyMessageBuffer[i].key.length() == 0)
        {
            PropertyMessageBuffer[i].key = key;
            PropertyMessageBuffer[i].value = value;
            breakMark = 1;
            break;
        }
    }
}

// 发送阶段
void AliyunIoTSDK::send(const char *param)
{
    Serial.println();
    char jsonBuf[1024];
    sprintf(jsonBuf, ALINK_BODY_FORMAT, CLIENT_ID, param);
    Serial.print("jsonBuf_len: ");
    Serial.println(strlen(jsonBuf));
    boolean d = client->publish(ALINK_TOPIC_PROP_POST, jsonBuf, strlen(jsonBuf) - 1);
    if (1 == d)
    {
        Serial.println("-----------PUSH MESSAGE------------");
        // Serial.println("消息内容: ");
        // serializeJsonPretty(PUSHdoc, Serial); // 串口美化打印json
        Serial.println(jsonBuf);
        Serial.println("-----------PUSH END------------");
    }
    else
    {
        Serial.println("PUSH Failed");
    }
    Serial.println();
}
void AliyunIoTSDK::send(char *key, float number)
{
    addMessageToBuffer(key, String(number));
    // messageBufferCheck();
}
void AliyunIoTSDK::send(char *key, int number)
{
    addMessageToBuffer(key, String(number));
    // messageBufferCheck();
}
void AliyunIoTSDK::send(char *key, double number)
{
    addMessageToBuffer(key, String(number));
    // messageBufferCheck();
}
void AliyunIoTSDK::send(char *key, char *text)
{
    addMessageToBuffer(key, "\"" + String(text) + "\"");
    // messageBufferCheck();
}

int AliyunIoTSDK::bindData(char *key, poniter_fun fp)
{
    size_t i;
    // Serial.print("bindData_key: ");
    // Serial.println(key);
    for (i = 0; i < DATA_CALLBACK_SIZE; i++)
    {
        // 会导致两个callback函数都被调用
        if (!poniter_array[i].fp)
        {
            poniter_array[i].key = key;
            poniter_array[i].fp = fp;
            // showPoniteArray();
            return 0;
        }
    }
    return -1;
}

int AliyunIoTSDK::unbindData(char *key)
{
    size_t i;
    for (i = 0; i < DATA_CALLBACK_SIZE; i++)
    {
        if (!strcmp(poniter_array[i].key, key))
        {
            poniter_array[i].key = NULL;
            poniter_array[i].fp = NULL;
            return 0;
        }
    }
    return -1;
}
