#include <Arduino.h>
#include <IOThing.h>



#define WIFI_SSID "preen"
#define WIFI_PASS "gumboots"
#define IOT_HOSTNAME "gnode1"
#define MQTT_SERVER "192.168.0.3"
#define NTP_SERVER "192.168.0.3"

IOThing iot(IOT_HOSTNAME);


void setup(){
  Serial.begin(115200);

  Serial.println("Configuring WiFi");
  iot.useWiFi(WIFI_SSID, WIFI_PASS);

  Serial.println("Configuring OTA");
  iot.useOTA();

  Serial.println("Configuring NTP");
  iot.useNTP(NTP_SERVER);

  Serial.println("Configuring MQTT");
  iot.useMQTT(MQTT_SERVER, [](String topic, String message){
    Serial.println("Got setting: " + topic + " = " + message);
  });
  Serial.println("Subscribing to 'test/things/#'");
  iot.topicSubscribe("test/things/#",[](String topic, String payload){
    Serial.println("Thing: " + topic + " = " + payload);
  });
  Serial.println("Subscribing to 'test/people/#'")
  iot.topicSubscribe("test/people/#",[](String topic, String payload){
    Serial.println("Person: " + topic + " = " + payload);
  });
}

void loop(){
  iot.loop();
}
