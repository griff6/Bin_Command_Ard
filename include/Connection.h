#include <Arduino.h>

void initiallizeMQTT();
void connectGSM();
void connectMQTT();
String calculateClientId();
String calculateJWT();
void MQTT_Poll();
void publishDataMessage();
void onMessageReceived(int messageSize);
