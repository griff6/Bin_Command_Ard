#include <Arduino.h>
#include <ArduinoJson.h>

typedef union{
  float floatingPoint;
  byte binary[4];
}binaryFloat;

void InitiallizeBluetooth();
void CheckBluetooth();
void PublishBluetoothDataMessage();
void SendMessage(String);
void SendDataRecord(String);
