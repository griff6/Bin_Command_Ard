#include <Arduino.h>

void SetupBinCables();
void GetBinCableValues();
void GetMinMaxAddresses();
double CalculateMoisture(float RH, int temperature);
uint8_t findDevices(int pin);
