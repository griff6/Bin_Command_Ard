#include <Arduino.h>
#include <ArduinoJson.h>

void ConnectWirelessNetwork();
bool WirelessConnected();
bool MQTTConnected();
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
void RequestSerialNumber();
void RequestTimeStamp();
void GetLocation();
void onMessageReceived(int messageSize);
void setRTCTime(DynamicJsonDocument doc);
void printWiFiStatus();
