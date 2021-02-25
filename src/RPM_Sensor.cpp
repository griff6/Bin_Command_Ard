/*
 * This is used to count the number of pulses from an IR sensor and determine an RPM from this.
 */
 #include "RPM_Sensor.h"
 #include "SharedResources.h"

//

int pulseCount = 0;
float rpm;
//float filteredRPM = 65536;
float rpm_fc = .6;//1;//.2;            // higher value puts less filtering

void SetupRPMSensor()
{
  pinMode(RPM_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RPM_PIN), RPM_Pulse, RISING);
}

void Calculate_RPM(unsigned long diffTime)
{
  rpm = pulseCount * 60000;
  rpm = (rpm / diffTime);
  filterValue(rpm, filteredValues.filteredRPM, rpm_fc);

  pulseCount = 0;
}
/*
void Print_RPM()
{
  String RPM = "RPM,";
  RPM += filteredValues.filteredRPM;
}
*/

void Save_RPM()
{
  String RPM = "RPM,";
  RPM += filteredValues.filteredRPM;
  //Serial.println(RPM);
}

//ISR whenever pin 2 goes high, indicating the black threshold was crossed.
void RPM_Pulse()
{
  pulseCount++;
}

float GetRPM()
{
  return rpm;
}
