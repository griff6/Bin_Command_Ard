#include <Arduino.h>

void SetupRPMSensor();
void Calculate_RPM(unsigned long diffTime);
void Print_RPM();
void Save_RPM();
void RPM_Pulse();
float GetRPM();
