#include "gsmConnection.h"
#include <ArduinoECCX08.h>
#include <utility/ECCX08JWS.h>
#include <ArduinoMqttClient.h>
#include <Arduino_JSON.h>
#include "arduino_secrets.h"
#include <MKRGSM.h>
#include <ArduinoJson.h>
#include "SharedResources.h"
#include <RTCZero.h>

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

RTCZero rtc;
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
  mqttClient.beginMessage("/devices/" + deviceId + "/events/DATA_SUMMARY");
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

void RequestTimeStamp() {
  // Set the time
  rtc.begin(); // initialize RTC
  // rtc.setTime(hours, minutes, seconds);
  // rtc.setDate(day, month, year);
  rtc.setEpoch(gsmAccess.getTime());

  Serial.print("Unix time = ");
  Serial.println(rtc.getEpoch());

  // Print date...
  print2digits(rtc.getDay());
  Serial.print("/");
  print2digits(rtc.getMonth());
  Serial.print("/");
  print2digits(rtc.getYear());
  Serial.print(" ");

  // ...and time
  print2digits(rtc.getHours());
  Serial.print(":");
  print2digits(rtc.getMinutes());
  Serial.print(":");
  print2digits(rtc.getSeconds());

  Serial.println();


/*
  // send message, the Print interface can be used to set the message contents
  mqttClient.beginMessage("/devices/" + deviceId + "/events/TIMESTAMP");

  mqttClient.print("Requesting Timestamp");

  mqttClient.endMessage();
  Serial.println();
  Serial.println("Requested TimeStamp.");
  */

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

void print2digits(int number) {
  if (number < 10) {
    Serial.print("0"); // print a 0 before if the number is < than 10
  }
  Serial.print(number);
}

/*

  Serial.println("Trying to get timestamp");

  Udp.begin(localPort);

  sendNTPpacket(); // send an NTP packet to a time server

  // wait to see if a reply is available
  delay(1000);

  if ( Udp.parsePacket() ) {
    Serial.println("packet received");

    // We've received a packet, read the data from it
    Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);

    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;

    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");

    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;

    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;

    // print Unix time:
    Serial.println(epoch);

    // print the hour, minute and second:
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    Serial.print(':');

    if ( ((epoch % 3600) / 60) < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }

    Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    Serial.print(':');

    if ( (epoch % 60) < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }

    Serial.println(epoch % 60); // print the second
  }
}

// send an NTP request to the time server at the given address
void sendNTPpacket()
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);

  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision

  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(timeServer, 123); //NTP requests are to port 123

  Udp.write(packetBuffer, NTP_PACKET_SIZE);

  Udp.endPacket();
}
*/

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
  else if(rcvMsg == "002")
  {

  }
}
