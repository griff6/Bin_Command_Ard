#include <Arduino.h>
#include <ArduinoJson.h>

void ConnectWirelessNetwork();
bool WirelessConnected();
void MaintainConnections();
bool connectWifi();
bool connectGSM();
void initiallizeMQTT();
void connectMQTT();
String calculateClientId();
String calculateJWT();
void MQTT_Poll();
void CheckConnection();
void publishDataMessage();
float Round(float var);
void publishEngineState();
void updateLiveData();
void RequestTimeStamp();
void GetLocation();
void onMessageReceived(int messageSize);
void setRTCTime(DynamicJsonDocument doc);
String print2digits(int number);
void printWiFiStatus();
