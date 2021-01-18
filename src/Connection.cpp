#include "Connection.h"
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
MqttClient    mqttClient(gsmSslClient);

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

void connectGSM() {
  if (gsmAccess.status() == GSM_READY || gprs.status() == GPRS_READY) {
    return;
  }
  Serial.println("Attempting to connect to the cellular network");

  while ((gsmAccess.begin(pinnumber) != GSM_READY) ||
         (gprs.attachGPRS(gprs_apn, gprs_login, gprs_password) != GPRS_READY)) {
    // failed, retry
    Serial.print(".");
    delay(1000);
  }

  Serial.println("You're connected to the cellular network");
  Serial.println();
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

//String calculateJWT() {
//  unsigned long now = getTime();
//
//  // calculate the JWT, based on:
//  //   https://cloud.google.com/iot/docs/how-tos/credentials/jwts
////  JSONVar jwtHeader;
////  JSONVar jwtClaim;
////
////  jwtHeader["alg"] = "ES256";
////  jwtHeader["typ"] = "JWT";
////
////  jwtClaim["aud"] = projectId;
////  jwtClaim["iat"] = now;
////  jwtClaim["exp"] = now + (24L * 60L * 60L); // expires in 24 hours
//
//  const size_t header_capacity = JSON_OBJECT_SIZE(2);
//  DynamicJsonDocument header_doc(header_capacity);
//  const size_t claim_capacity = JSON_OBJECT_SIZE(3);
//  DynamicJsonDocument claim_doc(header_capacity);
//
//  header_doc["alg"] = "ES256";
//  header_doc["typ"] = "JWT";
//
//  claim_doc["aud"] = projectId;
//  claim_doc["iat"] = now;
//  claim_doc["exp"] = now + (24L * 60L * 60L); // expires in 24 hours
//
//  char header[100];
//  serializeJson(header_doc, header);
//
//  char claim[100];
//  serializeJson(claim_doc, claim);
//
//  return ECCX08JWS.sign(0, header, claim);
//}

void publishDataMessage() {
  //Serial.println("Publishing message");
  String fanStatus = "OFF";

  if(filteredValues.filteredRPM > 100)
  {
    fanStatus = "ON";
  }

  // send message, the Print interface can be used to set the message contents
  mqttClient.beginMessage("/devices/" + deviceId + "/events");
  //mqttClient.beginMessage("/devices/" + deviceId + "/state");

  //https://arduinojson.org/v6/assistant/ gives some details on how to calculate the size
  //https://arduinojson.org/v5/assistant/
  //https://arduinojson.org/v5/faq/how-to-determine-the-buffer-size/
  //const size_t capacity = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5);

  const size_t capacity = JSON_OBJECT_SIZE(10);
  //DynamicJsonDocument doc(capacity);
  //DynamicJsonDocument doc(431);

  StaticJsonDocument<capacity> doc;
  Serial.print("JSON Document capacity:");
  Serial.println(capacity);

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

void onMessageReceived(int messageSize) {
  String rcvMsg;
  // we received a message, print out the topic and contents
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

  if(rcvMsg == "001")
    publishDataMessage();

}
