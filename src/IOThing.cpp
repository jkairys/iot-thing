#include <Arduino.h>
#include "IOThing.h"

IOThing::IOThing(char * hostname)
{
    strcpy(this->_hostname, hostname);
    //this->items = new TickerSchedulerItem[size];
    //this->size = size;
}

bool IOThing::_use_mqtt(){
  return this->_mqtt_state != IOT_MQTT_DISABLED;
}

bool IOThing::_use_ota(){
  return this->_ota_state == IOT_OTA_READY;
}

void IOThing::loop(){
  // task scheduler
  //ts.update();
  // OTA updates
  if(this->_use_ota()) ArduinoOTA.handle();
  // mqtt client
  if(this->_use_mqtt()) this->client.loop();
  if(this->_use_mqtt()) this->_reconnectMQTT();
  // ESP functions
  yield();
}

void IOThing::useOTA(){
  ArduinoOTA.setHostname(this->_hostname);
  ArduinoOTA.onStart([&]() {
    this->_ota_state = IOT_OTA_PROGRESS;
    //Serial.println("Start");
  });
  ArduinoOTA.onEnd([&]() {
    this->_ota_state = IOT_OTA_COMPLETE;
    //Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([&](unsigned int progress, unsigned int total) {
    this->_ota_state = IOT_OTA_PROGRESS;
    //Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([&](ota_error_t error) {
    this->_ota_state = IOT_OTA_ERROR;
    //Serial.printf("Error[%u]: ", error);
    //if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    //else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    //else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    //else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    //else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  this->_ota_state = IOT_OTA_READY;
}

void IOThing::useWiFi(char * ssid, char * password){
  Serial.println("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  Serial.println("Wifi begin for " + String(ssid) + ":"+ String(password));
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  Serial.println("Wifi OK");
}

void IOThing::_iot_settings_callback(String topic, String payload){
  if(topic == "dst") this->_dst = payload.toInt();
  if(topic == "timezone") this->_timezone = payload.toInt();
}

/*
  IOThing::_raw_mqtt_callback(topic, data, len)
  MQTT message received callback function, parse topic and match to
  one of our subscriptions. Once matched, run callback for this subscription,
  passing it the munged topic (sans. subscription substring) and payload.
*/
void IOThing::_raw_mqtt_callback(char * topic, byte * data, unsigned int len){
  // first munge the payload payload
  char buf[IOT_MQTT_PAYLOAD_MAXLEN+1] = "";
  int i = 0;
  for (i = 0; i < len; i++) {
    if(i >= IOT_MQTT_PAYLOAD_MAXLEN) break;
    buf[i] = (char)data[i];
  }
  buf[i] = '\0';
  String str_payload = String(buf);

  // now try to fit the topic to one of our subscriptions
  String str_topic = String(topic);

  // firstly, is it a settings call?
  String settings_topic = String(this->_hostname)+"/settings/";
  if(str_topic.startsWith(settings_topic)){
    //this is a setting
    str_topic.replace(settings_topic, "");
    this->_iot_settings_callback(str_topic, str_payload);
    this->_user_settings_callback(str_topic, str_payload);
  }

  int longest_match_len = 0;
  int longest_match_id = -1;
  String longest_match_sub = "";

  // find the longest subscription which matches this topic
  for(uint8_t i = 0; i < this->_subscriptionCount(); i++){
    String str_subscription = String(this->_mqtt_topics[i]);
    while(str_subscription.endsWith("#") || str_subscription.endsWith("/")){
      str_subscription = str_subscription.substring(0,str_subscription.length()-1);
    }
    int res = str_topic.indexOf(str_subscription);
    Serial.println("Compare " + String(topic) + " to pot: " + str_subscription);
    if(res != -1 && str_subscription.length() > longest_match_len) {
      longest_match_len = str_subscription.length();
      longest_match_id = i;
      longest_match_sub = str_subscription;
    }
  }

  // Check if we didn't match any of our topics (madness)
  if(longest_match_id == -1){
    // something went wrong, we should have been able to match this
    return;
  }

  // remove subscription from topic
  str_topic.replace(longest_match_sub, "");
  if(str_topic.startsWith("/")) str_topic = str_topic.substring(1, str_topic.length());


  // run callback function for this subscription
  // returning munged topic and payload as string
  this->_mqtt_callbacks[longest_match_id](str_topic, str_payload);
}

int IOThing::_mqtt_client_connect(){
  if(strlen(this->_mqtt_password)){
    return this->client.connect(this->_hostname, this->_mqtt_user, this->_mqtt_password);
  }else{
    return this->client.connect(this->_hostname);
  }
}

void IOThing::_reconnectMQTT(){
  if (!this->client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (this->_mqtt_client_connect()) {
      this->_mqtt_state = IOT_MQTT_CONNECTED;
      this->client.subscribe((String(this->_hostname) + "/settings/#").c_str());
    }
  }
}

void IOThing::useMQTT(char * server, uint16_t port, char * username, char * password, IOT_MQTT_CALLBACK_SIGNATURE settings_callback){
  strncpy(this->_mqtt_user, username, 32);
  strncpy(this->_mqtt_password, password, 32);
  this->_mqtt_port = port;
  this->useMQTT(server, settings_callback);
}

void IOThing::useMQTT(char * server, IOT_MQTT_CALLBACK_SIGNATURE settings_callback){ //char * topic, IOT_MQTT_CALLBACK_SIGNATURE callback){
  // setup MQTT connection
  this->client = PubSubClient(this->espClient);
  this->client.setServer(server, this->_mqtt_port ? this->_mqtt_port : 1883);
  this->client.setCallback([&](char *topic, byte *data, unsigned int len) {
    this->_raw_mqtt_callback(topic, data, len);
  });
  this->_user_settings_callback = settings_callback;
  // connect to MQTT server
  this->_reconnectMQTT();

}

// used for determining the enxt unused MQTT topic slot
int IOThing::_subscriptionCount(){
  for(int i = 0; i < IOT_MAX_MQTT_SUBSCRIPTIONS; i++){
    if(_mqtt_topics_map[i] == false){
      return i;
    }
  }
  return -1;
}

/* IOThing::topicSubscribe(topic, callback)
    subscribe to a new MQTT topic
    The callback function should be of the form fn(String topic, String payload)
    as it will be called whenever a message is received on this topic / subscription
*/
int IOThing::topicSubscribe(char * topic, IOT_MQTT_CALLBACK_SIGNATURE callback){
  // get the id of the next free slot
  byte next_subscription = this->_subscriptionCount();
  // if we're full, barf
  if(next_subscription >= IOT_MAX_MQTT_SUBSCRIPTIONS) return -1;
  // Store the topic and callback function for same
  strcpy(this->_mqtt_topics[next_subscription], topic);
  this->_mqtt_callbacks[next_subscription]= callback;
  this->_mqtt_topics_map[next_subscription] = true;
  // Subscribe to the topic
  this->client.subscribe(topic);
  return 0;
};

void IOThing::_log(char * msg){
  this->client.publish(
    (String(this->_hostname)+"/debug").c_str(),
    msg
  );
}

void IOThing::useNTP(char * server){
  // set status
  this->_ntp_state = IOT_NTP_NOSYNC;
  // callback to be run when time sync is obtained
  NTP.onNTPSyncEvent([&](NTPSyncEvent_t error) {
    if (error) {
      Serial.print("Time Sync error: ");
      if (error == noResponse)
        Serial.println("NTP server not reachable");
      else if (error == invalidAddress)
        Serial.println("Invalid NTP server address");
    }else {
      //Serial.print("Got NTP time: ");
      //Serial.println(NTP.getTimeDateString(NTP.getLastNTPSync()));
      this->_ntp_state = IOT_NTP_SYNC;
    }

  });
  NTP.begin(server, 1, false);
  NTP.setInterval(IOT_NTP_SYNC_INTERVAL);
  NTP.setTimeZone(this->_timezone + this->_dst);
}

void IOThing::publish(String topic, String value){
  char _value[64];
  value.toCharArray(_value, 64);
  this->publish(topic, _value);
}

void IOThing::publish(String topic, float value){
  char _value[64];
  String(value,2).toCharArray(_value, 64);
  this->publish(topic, _value);
}

void IOThing::publish(String topic, char * value){
  this->client.publish((String(this->_hostname)+ "/" + topic).c_str(), value);
}
