# iot-thing | ESP8266 - MQTT - NTP - OTA
A gentle introduction to the Internet of Things

## Overview
This is a software library to help you quickly build a microcontroller
project with internet connectivity for control and datalogging. The library has
been developed for use with the Arduino softwre using the WeMos D1 R2 ESP8266  
microcontroller.

## Features
The library wraps up a few other libraries into one neat package, so you can
focus on building your application and not worry about munging strings and
checking the time (unless that's your thing). The library combines:
* WiFi connectivity
* Over the air (OTA) updates
* MQTT messaging - multiple subscriptions, each with their own callback
* Network Time Protocol (NTP) time sync - so you don't need an RTC
* TickerScheduler - to call a number of functions at regular intervals

## Dependencies
This library requies the aforementioned libraries.

## Usage

~~~~
#include <IOThing.h>
#define WIFI_SSID "my-ssid"
#define WIFI_PASS "my-password"
#define IOT_HOSTNAME "my-first-thing"
#define MQTT_SERVER "192.168.0.1"
#define NTP_SERVER "192.168.0.1"

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
  Serial.println("Subscribing to 'test/people/#'");
  iot.topicSubscribe("test/people/#",[](String topic, String payload){
    Serial.println("Person: " + topic + " = " + payload);
  });

  // this should publish a message to my-first-thing/info
  iot.publish("info","Hello world!");
}

void loop(){
  iot.loop();
}

~~~~
