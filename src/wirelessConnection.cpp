#include "wirelessConnection.h"
#include <ArduinoECCX08.h>
#include <utility/ECCX08JWS.h>
#include <ArduinoMqttClient.h>
#include <Arduino_JSON.h>
#include "arduino_secrets.h"
#include <ArduinoJson.h>
#include "SharedResources.h"
#include "Engine_Control.h"
#include "RPM_Sensor.h"
#include "Bin_Pressure_Sensor.h"
#include "sdOperations.h"
#include <ArduinoUniqueID.h>

//#include <MKRGSM.h>
#include <WiFiNINA.h>

/////// Enter your sensitive data in arduino_secrets.h
const char pinnumber[]     = SECRET_PINNUMBER;
const char gprs_apn[]      = SECRET_GPRS_APN;
const char gprs_login[]    = SECRET_GPRS_LOGIN;
const char gprs_password[] = SECRET_GPRS_PASSWORD;

const char projectId[]     = SECRET_PROJECT_ID;
const char cloudRegion[]   = SECRET_CLOUD_REGION;
const char registryId[]    = SECRET_REGISTRY_ID;
//const String deviceId;      = SECRET_DEVICE_ID;
String deviceId;
const String projID       = SECRET_PROJECT_ID;

char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;     // the Wifi radio's status

const char broker[]        = "mqtt.googleapis.com";

WiFiSSLClient sslClient;
MqttClient mqttClient(sslClient);

bool locationSet = false;
bool wirelessConnected = false;
unsigned long wirelessInterval = 3600000;   //1 hour
unsigned long lastWirelessAttempt = 0;
unsigned long mqttConnectInterval = 900000; //15 getMinutes
unsigned long lastMQTTattempt = 0;

void ConnectWirelessNetwork()
{
  wirelessConnected = connectWifi();
  //wirelessConnected = connectGSM();

  //if(wirelessConnected)
  //  initiallizeMQTT();
}

//Check if the wireless connection is established
bool WirelessConnected()
{
  if(WiFi.status() == WL_CONNECTED){
    return true;
  }

  //if (gsmAccess.status() == GSM_READY || gprs.status() == GPRS_READY) {
  //  return true;
  //}

  return false;
}

bool MQTTConnected()
{
  if (!mqttClient.connected()) {
    // MQTT client is disconnected,
    return false;
  }
  return true;
}

void MaintainConnections()
{
  //If connection has been lost, keep trying to reconnect every hour
  if(!WirelessConnected() && lastWirelessAttempt < millis())
  {
    ConnectWirelessNetwork();
    lastWirelessAttempt = millis() + wirelessInterval;
  }

  if(wirelessConnected){
    //Serial.println(".");
    MQTT_Poll();
    //RequestTimeStamp();
  //  GetLocation();
    if(updateCloudEngineState)
    {
      publishEngineState();
    }
  }
}

//This is a dummy for the wifi
bool connectWifi()
{
  int connectAttempts = 0;

  if(WiFi.status() == WL_CONNECTED){
    return true;
  }

  // Check for the WiFi module
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    //while (true);
    return false;
  }

  Serial.print("WiFi firmware version ");
  Serial.println(WiFi.firmwareVersion());

  Serial.print("Attempting to connect to SSID: ");
  Serial.print(SECRET_SSID);
  Serial.print(" ");

  while (WiFi.begin(SECRET_SSID, SECRET_PASS) != WL_CONNECTED) {
    // spend 15 seconds trying to connect and then give up
    if(connectAttempts > 5)
    {
      Serial.println("Could not connect to the wifi");
      return false;
    }

    connectAttempts++;

    Serial.print(".");
    delay(3000);
  }

  Serial.println("Connected to WiFi");
  printWiFiStatus();

  initiallizeMQTT();

  return true;
}

/*
//This is actually for the GSM
bool connectGSM() {
  bool gsmConnected = false;
  int counter = 0;

  if (gsmAccess.status() == GSM_READY || gprs.status() == GPRS_READY) {
    return true;
  }
  Serial.println("Attempting to connect to the cellular network");

  while(((counter < 3) && ((gsmAccess.begin(pinnumber) != GSM_READY))) ||
         (gprs.attachGPRS(gprs_apn, gprs_login, gprs_password) != GPRS_READY)) {
    // failed, retry
    //Serial.print(counter++);
    Serial.println("Failed Connection Attempt");
    counter++;
    //gsmConnected = false;

    if(counter > 30)
    {
      return false;
    }
    counter++;

    delay(1000);
  }

  if((gsmAccess.begin(pinnumber) != GSM_READY) ||
         (gprs.attachGPRS(gprs_apn, gprs_login, gprs_password) != GPRS_READY))
         {
           gsmConnected = false;
           Serial.println("Failed to connect to GSM");
           return gsmConnected;
         }


    location.begin();
    gsmConnected = true;

    Serial.println("You're connected to the cellular network");
    Serial.println();

  initiallizeMQTT();

  return gsmConnected;
}
*/


void initiallizeMQTT()
{
    if (!ECCX08.begin()) {
    Serial.println("No ECCX08 present!");
    //while (1);
  }

  deviceId = "W";
  for (size_t i = 0; i < UniqueIDsize; i++)
  {
    if (UniqueID[i] < 0x10)
      deviceId += "0";
      //Serial.print("0");
    //Serial.print(UniqueID[i], HEX);
    deviceId += String(UniqueID[i], HEX);
    //Serial.print(" ");
  }

  Serial.print("Device ID: ");
  Serial.println(deviceId);
  // Calculate and set the client id used for MQTT
  String clientId = calculateClientId();

  mqttClient.setId(clientId);

  // Set the message callback, this function is
  // called when the MQTTClient receives a message
  mqttClient.onMessage(onMessageReceived);
}

unsigned long getTime() {
  // get the current time from the cellular module
//  return gsmAccess.getTime();
  return WiFi.getTime();
}

void connectMQTT() {
  int attempts = 0;

  if (mqttClient.connected()) {
    // MQTT client is connected already
    return;
  }

  Serial.print("Attempting to connect to MQTT broker: ");
  Serial.print(broker);
  Serial.println(" ");

  while (!mqttClient.connected()) {
    // Calculate the JWT and assign it as the password
    String jwt = calculateJWT();

    mqttClient.setUsernamePassword("", jwt);
    Serial.println("Connecting...");

    if (!mqttClient.connect(broker, 8883)) {
      // failed, retry
      //Serial.println(".");
      //delay(500);
      Serial.println("Failed to Connect to MQTT broker");
      return;
    }

    if(attempts++ >= 2)
    {
      Serial.println("Failed to Connect to MQTT broker");
      return;
    }
  }
  Serial.println();

  Serial.println("You're connected to the MQTT broker");
  Serial.print("Device ID: ");
  Serial.println(deviceId);
  Serial.print("Project ID: ");
  Serial.println(projID);

  Serial.println();

  // subscribe to topics
  mqttClient.subscribe("/devices/" + deviceId + "/config", 1);
  mqttClient.subscribe("/devices/" + deviceId + "/commands/#");


  if(!timeIsSet)
    RequestTimeStamp();

  publishEngineState();

  //mqttClient.subscribe("/devices/" + deviceId + "/commands/#");
  //mqttClient.subscribe("/projects/" + projID + "/subscriptions/my-sub");
}

String calculateClientId() {
  String clientId;

  // Format:
  //
  //   projects/{project-id}/locations/{cloud-region}/registries/{registry-id}/devices/{device-id}
  //

  clientId += "projects/";
  clientId += projectId;
  clientId += "/locations/";
  clientId += cloudRegion;
  clientId += "/registries/";
  clientId += registryId;
  clientId += "/devices/";
  clientId += deviceId;

  return clientId;
}

String calculateJWT() {
  unsigned long now = getTime();

  // calculate the JWT, based on:
  //   https://cloud.google.com/iot/docs/how-tos/credentials/jwts
  JSONVar jwtHeader;
  JSONVar jwtClaim;

  jwtHeader["alg"] = "ES256";
  jwtHeader["typ"] = "JWT";

  jwtClaim["aud"] = projectId;
  jwtClaim["iat"] = now;
  jwtClaim["exp"] = now + (24L * 60L * 60L); // expires in 24 hours

  return ECCX08JWS.sign(0, JSON.stringify(jwtHeader), JSON.stringify(jwtClaim));
}

void MQTT_Poll()
{
  //CheckConnection();

  if(!WirelessConnected())
    return;

  if(!mqttClient.connected() && lastMQTTattempt < millis())
  {
    connectMQTT();
    lastMQTTattempt = millis() + mqttConnectInterval;
  }

  //if (!mqttClient.connected()) {

    // MQTT client is disconnected, connect
//    connectMQTT();
//  }

  if(mqttClient.connected())
    mqttClient.poll();
}

void CheckConnection()
{
  //if (gsmAccess.status() != GSM_READY || gprs.status() != GPRS_READY) {
  //  connectGSM();
  //}
}

void publishDataMessage(DynamicJsonDocument doc, bool tempCable) {

  if(!wirelessConnected){
    return;
  }

  mqttClient.beginMessage("/devices/" + deviceId + "/events/DATA_SUMMARY");
  Serial.println("Writing data to Google Firestore");

  if(tempCable)
  {
    for(int i=0; i<10; i++ )
    {
      MQTT_Poll();
      delay(1000);
    }
  }

  //serializeJson(doc, mqttClient);
  serializeJson(doc, mqttClient);
  mqttClient.endMessage();

  //serializeJsonPretty(doc, Serial);
  //Serial.println();
}

void publishEngineState() {

  //if(lastEngineUpdate == engineState)
  //{
//    return;
//  }

if(!WirelessConnected())
  return;

  Serial.println();
  Serial.println("Publishing engine state");
  mqttClient.beginMessage("/devices/" + deviceId + "/events/ENGINE_STATE");

  const size_t capacity = JSON_OBJECT_SIZE(1);
  //DynamicJsonDocument doc(capacity);
  //DynamicJsonDocument doc(431);

  StaticJsonDocument<capacity> doc;

  doc["fs"] = engineState;


  serializeJson(doc, mqttClient);
  mqttClient.endMessage();
  serializeJsonPretty(doc, Serial);
  Serial.println();

//  lastEngineUpdate = engineState;

  updateCloudEngineState = false;
}

void updateLiveData() {

  Serial.println();
  Serial.println("Publishing live data");
  mqttClient.beginMessage("/devices/" + deviceId + "/events/LIVE_DATA");

  const size_t capacity = JSON_OBJECT_SIZE(6);
  //DynamicJsonDocument doc(capacity);
  //DynamicJsonDocument doc(431);

  StaticJsonDocument<capacity> doc;

  GetPressure();
  GetBatteryVoltage();

  doc["rpm"] = Round(GetRPM());//Round(filteredValues.filteredRPM);
  doc["sp"] = Round(filteredValues.filteredSP);
  doc["eh"] = Round(config.engineTime);
  doc["bv"] = Round(batteryVoltage);
  doc["ft"] = Round(filteredValues.filteredFanTemp);
  doc["fh"] = Round(filteredValues.filteredFanHumidity);

  serializeJson(doc, mqttClient);
  mqttClient.endMessage();
  serializeJsonPretty(doc, Serial);
  Serial.println();
}


void RequestTimeStamp() {
  mqttClient.beginMessage("/devices/" + deviceId + "/events/TIMESTAMP");

  //mqttClient.print("Requesting Timestamp");
  mqttClient.print("");

  mqttClient.endMessage();
  Serial.println();
  Serial.println("Requested TimeStamp.");
}

/*
void GetLocation()
{
  if (!locationSet && location.available() && location.longitude() != -95.00) {
    locationSet = true;

    Serial.println("Getting Location");

    // send message, the Print interface can be used to set the message contents
    mqttClient.beginMessage("/devices/" + deviceId + "/events/LOCATION");

    const size_t capacity = JSON_OBJECT_SIZE(3);

    StaticJsonDocument<capacity> doc;

    doc["longitude"] = location.longitude();
    doc["latitude"] = location.latitude();
    doc["altitude"] = location.altitude();

    serializeJson(doc, mqttClient);

    mqttClient.endMessage();
    serializeJsonPretty(doc, Serial);

    Serial.print("Location: ");
    Serial.print(location.latitude(), 7);
    Serial.print(", ");
    Serial.println(location.longitude(), 7);

    Serial.print("Altitude: ");
    Serial.print(location.altitude());
    Serial.println("m");

    Serial.print("Accuracy: +/- ");
    Serial.print(location.accuracy());
    Serial.println("m");

    Serial.println();
  }
}
*/

void onMessageReceived(int messageSize) {
  String rcvMsg;
  // we received a message, print out the topic and contents
  Serial.println();
  Serial.print("Received a message with topic '");
  Serial.print(mqttClient.messageTopic());
  Serial.print("', length ");
  Serial.print(messageSize);
  Serial.println(" bytes:");

  // use the Stream interface to print the contents
  while (mqttClient.available()) {
    //Serial.print((char)mqttClient.read());
    rcvMsg += (char)mqttClient.read();
  }
  Serial.print("MQTT Message Received: ");
  Serial.println(rcvMsg);
  Serial.println();

  //char json[] = rcvMsg;
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, rcvMsg);
  const char* command = doc["msg"];
  const char* attribute1 = doc["attribute1"];
  Serial.print("msg: ");
  Serial.println(command);

  if(strcmp(command, "001") == 0)           //Received Request for data message
    ProcessDataRecord();
  else if(strcmp(command, "002") == 0)    //Request for live data
  {
    updateLiveData();
  }else if(strcmp(command, "003") == 0)   //Received the server time
  {
    setRTCTime(doc);
  }else if(strcmp(command, "004") == 0)   //Turn Engine OFF
  {
    userEngineCommand = OFF;
    Serial.println();
    //Serial.println("Received command to turn engine OFF (gsmConnection - onMesssageReceived())");
  }else if(strcmp(command, "005") == 0)   //Turn Engine ON
  {
    startEngine = true;
    starterAttempt = 0;
    userEngineCommand = ON;
    Serial.println();
  //Serial.println("Received command to turn engine ON (gsmConnection - onMesssageReceived())");
    //Serial.print("Set userEngineCommand to: ");
    //Serial.println(userEngineCommand);
  }else if(strcmp(command, "006") == 0)   //Turn Engine to AUTO
  {
    //TODO: Handle when the engine gets changed to AUTO
  }else if(strcmp(command, "007") == 0)      //Start new currentBatch
  {
    Serial.println();
    Serial.print("Received start new batch command with grain: ");
    Serial.println(attribute1);
    StartNewBatch(attribute1);
  }else if(strcmp(command, "008") == 0)     //message with the device serial number
  {

  }
}

void setRTCTime(DynamicJsonDocument doc)
{
  //String t1 = doc["time"];
  //Serial.print("T1: ");
  //Serial.println(t1);

  unsigned long time = doc["time"];
  //Serial.print("Time: ");
  //Serial.println(time);

  hourlyData = true;
  timeIsSet = true;
  rtc.begin();
  rtc.setEpoch(time);
  SetRTC_Alarm();
  rtc.enableAlarm(rtc.MATCH_HHMMSS);
  rtc.attachInterrupt(RTC_Alarm);
/*
  Serial.print("RTC TIME: ");
  print2digits(rtc.getHours());
  Serial.print(":");
  print2digits(rtc.getMinutes());
  Serial.print(":");
  print2digits(rtc.getSeconds());
  Serial.println();
  */
}

void printWiFiStatus() {
  // print your WiFi IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}
