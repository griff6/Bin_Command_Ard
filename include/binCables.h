#include <Arduino.h>

void SetupBinCables();
void GetBinCableValues();
float CalculateMoisture(float RH, int temperature);
uint8_t findDevices(int pin);
