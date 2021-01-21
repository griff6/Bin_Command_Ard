#include "gsmConnection.h"
#include <ArduinoECCX08.h>
#include <utility/ECCX08JWS.h>
#include <ArduinoMqttClient.h>
#include <Arduino_JSON.h>
#include "arduino_secrets.h"
#include <MKRGSM.h>
#include <ArduinoJson.h>
#include "SharedResources.h"

/////// Enter your sensitive data in arduino_secrets.h
const char pinnumber[]     = SECRET_PINNUMBER;
const char gprs_apn[]      = SECRET_GPRS_APN;
const char gprs_login[]    = SECRET_GPRS_LOGIN;
const char gprs_password[] = SECRET_GPRS_PASSWORD;

const char projectId[]     = SECRET_PROJECT_ID;
const char cloudRegion[]   = SECRET_CLOUD_REGION;
const char registryId[]    = SECRET_REGISTRY_ID;
const String deviceId      = SECRET_DEVICE_ID;
const String projID       = SECRET_PROJECT_ID;

const char broker[]        = "mqtt.googleapis.com";

GSM gsmAccess;
GPRS gprs;
GSMSSLClient  gsmSslClient;
GSMLocation location;
MqttClient    mqttClient(gsmSslClient);

//RTCZero rtc;
bool locationSet = false;

/*
GSMUDP Udp;
unsigned int localPort = 2390;      // local port to listen for UDP packets
IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
*/

void initiallizeMQTT()
{
    if (!ECCX08.begin()) {
    Serial.println("No ECCX08 present!");
    while (1);
  }

  // Calculate and set the client id used for MQTT
  String clientId = calculateClientId();

  mqttClient.setId(clientId);

  // Set the message callback, this function is
  // called when the MQTTClient receives a message
  mqttClient.onMessage(onMessageReceived);
}

unsigned long getTime() {
  // get the current time from the cellular module
  return gsmAccess.getTime();
}

bool connectGSM() {
  bool gsmConnected = false;
  int counter = 0;

  if (gsmAccess.status() == GSM_READY || gprs.status() == GPRS_READY) {
    return true;
  }
  Serial.println("Attempting to connect to the cellular network");

  while((counter < 3 && (gsmAccess.begin(pinnumber) != GSM_READY) ||
         (gprs.attachGPRS(gprs_apn, gprs_login, gprs_password) != GPRS_READY))) {
    // failed, retry
    //Serial.print(counter++);
    Serial.println("Failed Connection Attempt");
    counter++;
    //gsmConnected = false;
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


  return gsmConnected;
}

void connectMQTT() {
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
      Serial.println(".");
      delay(5000);
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

  RequestTimeStamp();

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
  if (gsmAccess.status() != GSM_READY || gprs.status() != GPRS_READY) {
    connectGSM();
  }

  if (!mqttClient.connected()) {
    // MQTT client is disconnected, connect
    connectMQTT();

  }

  mqttClient.poll();
}

void publishDataMessage() {

  int fanStatus = 0;

  if(filteredValues.filteredRPM > 100)
  {
    fanStatus = 1;
  }

  // send message, the Print interface can be used to set the message contents
  //mqttClient.beginMessage("/devices/" + deviceId + "/events/DATA_SUMMARY");
  Serial.println();
  Serial.println("Publishing message");
  mqttClient.beginMessage("/devices/" + deviceId + "/events/DATA_SUMMARY");

  //https://arduinojson.org/v6/assistant/ gives some details on how to calculate the size
  //https://arduinojson.org/v5/assistant/
  //https://arduinojson.org/v5/faq/how-to-determine-the-buffer-size/
  //const size_t capacity = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5);
  const size_t capacity = JSON_OBJECT_SIZE(15);
  //DynamicJsonDocument doc(capacity);
  //DynamicJsonDocument doc(431);

  StaticJsonDocument<capacity> doc;

  doc["ap"] = filteredValues.filteredAirPressure;
  doc["ah"] = filteredValues.filteredAirHumidity;
  doc["at"] = filteredValues.filteredAirTemp;
  doc["fs"] = fanStatus;
  doc["fh"] = filteredValues.filteredFanHumidity;
  doc["ft"] = filteredValues.filteredFanTemp;
  doc["sp"] = filteredValues.filteredSP;
  doc["minT"] = 65536;
  doc["maxT"] = 65536;
  doc["avT"] = 65536;
  doc["minM"] = 65536;
  doc["maxM"] = 65536;
  doc["avM"] = 65536;
  doc["mode"] = 1;
  doc["bn"] = currentBatch;

  serializeJson(doc, mqttClient);
  mqttClient.endMessage();
  serializeJsonPretty(doc, Serial);
  Serial.println();
  //Serial.println("Published Message.");
}

void publishMaxMins() {
  // send message, the Print interface can be used to set the message contents
  mqttClient.beginMessage("/devices/" + deviceId + "/events/MAX_MIN");
  //mqttClient.beginMessage("/devices/" + deviceId + "/state");

  const size_t capacity = JSON_OBJECT_SIZE(10);
  StaticJsonDocument<capacity> doc;

  //doc["rpm"] = filteredValues.filteredRPM;
  doc["staticPressure"] = filteredValues.filteredSP;
  //doc["engineHours"] = filteredValues.engineTime;

  doc["ambient_temperature"] = filteredValues.filteredAirTemp;
  doc["ambient_humidity"] = filteredValues.filteredAirHumidity;
  doc["ambient_airPressure"] = filteredValues.filteredAirPressure;

  doc["fan_temperature"] = filteredValues.filteredFanTemp;
  doc["fan_humidity"] = filteredValues.filteredFanHumidity;
  doc["grain"] = "dummyGrain";
  doc["fanStatus"] = "OFF";

  doc["grain_temperature"] = 0;
  doc["grain_moisture"] = 0;
  //doc["dummy"] = -1;

  //serializeJson(doc, Serial);

  //mqttClient.print("hello ");
  serializeJson(doc, mqttClient);
  //mqttClient.print(millis());
  mqttClient.endMessage();
  serializeJsonPretty(doc, Serial);
  Serial.println();
  Serial.println("Published Message.");
}

void RequestTimeStamp() {
  mqttClient.beginMessage("/devices/" + deviceId + "/events/TIMESTAMP");

  //mqttClient.print("Requesting Timestamp");
  mqttClient.print("");

  mqttClient.endMessage();
  Serial.println();
  Serial.println("Requested TimeStamp.");
}

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
  Serial.print("msg: ");
  Serial.println(command);

  if(strcmp(command, "001") == 0)           //Received Request for data message
    publishDataMessage();
  else if(strcmp(command, "002") == 0)
  {

  }else if(strcmp(command, "003") == 0)
  {
    setRTCTime(doc);         //Received the server time
  }
}

void setRTCTime(DynamicJsonDocument doc)
{
  unsigned long time = doc["time"];
  Serial.print("Time: ");
  Serial.println(time);

  hourlyData = true;
  rtc.begin();
  rtc.setEpoch(time);
  SetRTC_Alarm();
  rtc.enableAlarm(rtc.MATCH_HHMMSS);
  rtc.attachInterrupt(RTC_Alarm);

  Serial.print("RTC TIME: ");
  print2digits(rtc.getHours());
  Serial.print(":");
  print2digits(rtc.getMinutes());
  Serial.print(":");
  print2digits(rtc.getSeconds());
  Serial.println();
}


void print2digits(int number) {
  if (number < 10) {
    Serial.print("0"); // print a 0 before if the number is < than 10
  }
  Serial.print(number);
}