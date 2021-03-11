#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SHA256.h>

#define CHECK_INTERVAL 10000
#define MESSAGE_BUFFER_SIZE 10
#define RETRY_CRASH_COUNT 5

static const char *deviceName = NULL;
static const char *productKey = NULL;
static const char *deviceSecret = NULL;
static const char *region = NULL;

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

static bool mqttConnecting = false;
void (*resetFunc)(void) = 0; //制造重启命令
void AliyunIoTSDK::mqttCheckConnect()
{
    if (client != NULL && !mqttConnecting)
    {
        if (!client->connected())
        {
            client->disconnect();
            Serial.println("Connecting to MQTT Server ...");
            mqttConnecting = true;
            if (client->connect(clientId, mqttUsername, mqttPwd))
            {
                Serial.println("MQTT Connected!");
            }
            else
            {
                Serial.print("MQTT Connect err:");
                Serial.println(client->state());
                retry_count++;
                if (retry_count > RETRY_CRASH_COUNT)
                {
                    resetFunc();
                }
            }
            mqttConnecting = false;
        }
        else
        {
            // Serial.println("state is connected");
            retry_count = 0;
        }
    }
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