#include <Arduino.h>

void initiallizeMQTT();
bool connectGSM();
void connectMQTT();
String calculateClientId();
String calculateJWT();
void MQTT_Poll();
void publishDataMessage();
void RequestTimeStamp();
void print2digits(int number);
void GetLocation();
void onMessageReceived(int messageSize);
