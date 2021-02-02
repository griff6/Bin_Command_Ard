#include "gsmConnection.h"
#include <ArduinoECCX08.h>
#include <utility/ECCX08JWS.h>
#include <ArduinoMqttClient.h>
#include <Arduino_JSON.h>
#include "arduino_secrets.h"
#include <MKRGSM.h>
#include <ArduinoJson.h>
#include "SharedResources.h"
#include "Engine_Control.h"

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

  while(((counter < 3) && ((gsmAccess.begin(pinnumber) != GSM_READY))) ||
         (gprs.attachGPRS(gprs_apn, gprs_login, gprs_password) != GPRS_READY)) {
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
  String cableNum = "C0";

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
  //const size_t capacity;// = JSON_OBJECT_SIZE(20);// + JSON_OBJECT_SIZE(16*numCables);
  //DynamicJsonDocument doc(capacity);
  //DynamicJsonDocument doc(431);
  //size_t capacity = JSON_OBJECT_SIZE(78);

  //if(numCables == 0)
//    capacity = JSON_OBJECT_SIZE(20);
//  else
//    capacity = JSON_OBJECT_SIZE(20);

  String ts = "";
  ts = ts + print2digits(rtc.getYear());
  ts += "-";
  ts += print2digits(rtc.getMonth());
  ts += "-";
  ts += print2digits(rtc.getDay());
  ts += " ";
  ts += print2digits(rtc.getHours());
  ts += ":";
  ts += print2digits(rtc.getMinutes());

  //StaticJsonDocument<capacity> doc;
  size_t capacity = JSON_OBJECT_SIZE(78);
  DynamicJsonDocument doc(capacity);

  doc["ts"] = ts;
  doc["msg"] = 0;
  doc["ap"] = filteredValues.filteredAirPressure;
  doc["ah"] = filteredValues.filteredAirHumidity;
  doc["at"] = filteredValues.filteredAirTemp;
  doc["fs"] = fanStatus;
  doc["fh"] = filteredValues.filteredFanHumidity;
  doc["ft"] = filteredValues.filteredFanTemp;
  doc["sp"] = filteredValues.filteredSP;//serialized(String(filteredValues.filteredSP,1));
  doc["minT"] = 0;
  doc["maxT"] = 0;
  doc["avT"] = 0;
  doc["minM"] = 0;
  doc["maxM"] = 0;
  doc["avM"] = 0;
  doc["mode"] = fanMode;
  doc["bn"] = config.batchNumber;
  doc["ber"] = config.batchEngineTime;
  doc["bat"] = config.batchAerateTime;
  doc["bdt"] = config.batchDryingTime;
  doc["gn"] = config.grain;
  doc["nc"] = numCables;

  serializeJson(doc, mqttClient);
  mqttClient.endMessage();
  serializeJsonPretty(doc, Serial);
  Serial.println();



  for(int cable = 0; cable < numCables; cable++)
  {
    DynamicJsonDocument doc(capacity);

    doc["ts"] = ts;
    doc["msg"] = cable+1;

    if(cable == 0)
      cableNum = "C0T";
    else if(cable == 1)
      cableNum = "C1T";
    else if(cable == 2)
      cableNum == "C2T";

    JsonArray cableTemp = doc.createNestedArray(cableNum);

    for(int i = 0; i < cableValues[cable].numSensors; i++)
    {
      /*
      if(cable == 0)
        cableNum = "C0T";
      else if(cable == 1)
        cableNum = "C1T";
      else if(cable == 2)
        cableNum == "C2T";

      cableNum += i;

      doc[cableNum] = cableValues[cable].sensors[i].temperature;
      */
      cableTemp.add(cableValues[cable].sensors[i].temperature);
    }



    if(cable == 0)
      cableNum = "C0M";
    else if(cable == 1)
      cableNum = "C1M";
    else if(cable == 2)
      cableNum == "C2M";

    JsonArray cableMois = doc.createNestedArray(cableNum);

    for(int i = 0; i < cableValues[cable].numSensors; i++)
    {
      /*
      if(cable == 0)
        cableNum = "C0M";
      else if(cable == 1)
        cableNum = "C1M";
      else if(cable == 2)
        cableNum == "C2M";

      cableNum += i;

      doc[cableNum] = cableValues[cable].sensors[i].moisture;
      */
      cableMois.add(cableValues[cable].sensors[i].moisture);
    }

    serializeJson(doc, mqttClient);
    mqttClient.endMessage();
    serializeJsonPretty(doc, Serial);
    Serial.println();
  }



//  Serial.print("Crop in publishDataMessage(): ");
  //Serial.println(config.grain);

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

void publishEngineState() {

  Serial.println();
  Serial.println("Publishing engine state");
  mqttClient.beginMessage("/devices/" + deviceId + "/events/ENGINE_STATE");

  const size_t capacity = JSON_OBJECT_SIZE(1);
  //DynamicJsonDocument doc(capacity);
  //DynamicJsonDocument doc(431);

  StaticJsonDocument<capacity> doc;

  //if(engineState == STOPPED || engineState == STARTING)
    //doc["fs"] = 1;
  //else if(engineState == RUNNING)
    //doc["fs"] = 0;

  doc["fs"] = engineState;


  serializeJson(doc, mqttClient);
  mqttClient.endMessage();
  serializeJsonPretty(doc, Serial);
  Serial.println();

  updateEngineState = false;
}

void updateLiveData() {

  Serial.println();
  Serial.println("Publishing live data");
  mqttClient.beginMessage("/devices/" + deviceId + "/events/LIVE_DATA");

  const size_t capacity = JSON_OBJECT_SIZE(3);
  //DynamicJsonDocument doc(capacity);
  //DynamicJsonDocument doc(431);

  StaticJsonDocument<capacity> doc;

  doc["rpm"] = filteredValues.filteredRPM;
  doc["sp"] = filteredValues.filteredSP;
  doc["eh"] = config.engineTime;

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
  const char* attribute1 = doc["attribute1"];
  Serial.print("msg: ");
  Serial.println(command);

  if(strcmp(command, "001") == 0)           //Received Request for data message
    publishDataMessage();
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


String print2digits(int number) {
  String ret = "";
  ret = ret + number;

  if (number < 10) {
    //Serial.print("0"); // print a 0 before if the number is < than 10
    ret = "0" + ret;
  }
  //Serial.print(number);
  return ret;
}
