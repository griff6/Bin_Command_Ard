#include <Arduino.h>
#include <RTCZero.h>
#include <ArduinoJson.h>

void InitiallizeSD();
void GetConfigFile();
void SaveConfigFile();
void SaveDataRecord(DynamicJsonDocument doc);
void GetDataRecord(unsigned long input);
