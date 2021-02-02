#include <Arduino.h>
#include <ArduinoJson.h>

void initiallizeMQTT();
bool connectGSM();
void connectMQTT();
String calculateClientId();
String calculateJWT();
void MQTT_Poll();
void publishDataMessage();
void publishEngineState();
void updateLiveData();
void RequestTimeStamp();
void GetLocation();
void onMessageReceived(int messageSize);
void setRTCTime(DynamicJsonDocument doc);
String print2digits(int number);
