#include <Arduino.h>
#include <ArduinoJson.h>

void initiallizeMQTT();
bool connectGSM();
void connectMQTT();
String calculateClientId();
String calculateJWT();
void MQTT_Poll();
void publishDataMessage();
void RequestTimeStamp();
void GetLocation();
void onMessageReceived(int messageSize);
void setRTCTime(DynamicJsonDocument doc);
void print2digits(int number);
