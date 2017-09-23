#include <stdint.h>
#include <functional>


#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <TimeLib.h>
#include <NtpClientLib.h>
#include <TickerScheduler.h>

#define IOT_WIFI_DISABLED 0
#define IOT_WIFI_DISCONNECTED 1
#define IOT_WIFI_CONNECTING 2
#define IOT_WIFI_CONNECTED 3

#define IOT_NTP_SYNC_INTERVAL 300
#define IOT_NTP_DISABLED 0
#define IOT_NTP_NOSYNC 1
#define IOT_NTP_SYNC 2

#define IOT_MQTT_DISABLED 0
#define IOT_MQTT_DISCONNECTED 1
#define IOT_MQTT_CONNECTING 2
#define IOT_MQTT_CONNECTED 3

#define IOT_OTA_DISABLED 0
#define IOT_OTA_READY 1
#define IOT_OTA_PROGRESS 2
#define IOT_OTA_COMPLETE 3
#define IOT_OTA_ERROR 4

#define IOT_MQTT_CALLBACK_SIGNATURE std::function<void(String t, String  p)>
#define IOT_MQTT_TOPIC_MAXLEN 32
#define IOT_MQTT_PAYLOAD_MAXLEN 32
#define IOT_MAX_MQTT_SUBSCRIPTIONS 5

class IOThing
{
private:
  uint8_t _ota_state;
  uint8_t _wifi_state;
  uint8_t _mqtt_state;
  uint8_t _ntp_state;

  WiFiClient espClient;
  PubSubClient client;
  TickerScheduler * ts;

  char _hostname[16];

  bool _use_ota();
  bool _use_mqtt();
  bool _use_ntp();
  int _subscriptionCount();
  void _log(char * msg);

  //char _mqtt_topic[IOT_MQTT_TOPIC_MAXLEN];
  IOT_MQTT_CALLBACK_SIGNATURE _callback;
  void _raw_mqtt_callback(char * topic, byte* payload, unsigned int length);
  void _reconnectMQTT();
  int _mqtt_client_connect();

  void _iot_settings_callback(String topic, String payload);
  IOT_MQTT_CALLBACK_SIGNATURE _user_settings_callback;

  char _ssid[32];
  char _password[32];
  char _mqtt_topics[IOT_MAX_MQTT_SUBSCRIPTIONS][IOT_MQTT_TOPIC_MAXLEN];
  IOT_MQTT_CALLBACK_SIGNATURE _mqtt_callbacks[IOT_MAX_MQTT_SUBSCRIPTIONS];

  bool _mqtt_topics_map[IOT_MAX_MQTT_SUBSCRIPTIONS];

  char _mqtt_user[32];
  char _mqtt_password[32];
  uint16_t _mqtt_port;


public:
  int8_t _timezone = 10;
  bool _dst = 1;

  IOThing(char * hostname);
  void loop();
  void useWiFi(char * ssid, char * password);
  void useOTA();
  void useMQTT(char * server, IOT_MQTT_CALLBACK_SIGNATURE settings_callback);
  void useMQTT(char * server, uint16_t port, char * username, char * password, IOT_MQTT_CALLBACK_SIGNATURE settings_callback);
   //, char * topic,  IOT_MQTT_CALLBACK_SIGNATURE callback);
  void useNTP(char * server);
  int topicSubscribe(char * topic, IOT_MQTT_CALLBACK_SIGNATURE);
  void publish(String topic, String value);
  void publish(String topic, float value);
  void publish(String topic, char * value);
  void publish(String topic, String value, bool retained);
  void publish(String topic, char * value, bool retained);
  void _log(String msg);
  bool ntpSynced();
};

/*
void tickerFlagHandle(volatile bool * flag);

typedef std::function<void(void)> tscallback_t;

struct TickerSchedulerItem
{
    Ticker t;
    volatile bool flag = false;
    tscallback_t cb;
    uint32_t period;
    volatile bool is_used = false;
};

class TickerScheduler
{
private:
    uint size;
    TickerSchedulerItem *items = NULL;

    void handleTicker(tscallback_t, volatile bool * flag);

public:
    TickerScheduler(uint size);
    ~TickerScheduler();

    bool add(uint i, uint32_t period, tscallback_t, boolean shouldFireNow = false);
    bool remove(uint i);
    bool enable(uint i);
    bool disable(uint i);
    void enableAll();
    void disableAll();
    void update();
};
*/
