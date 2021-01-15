#include "Bin_Pressure_Sensor.h"
#include "SharedResources.h"

#define PRESSURE_PIN A1  //Pin for the pressure sensor

float sp_fc = .75;//.2;            // higher value puts less filtering
float static_pressure = 0;

void SetupPressure()
{
  pinMode(PRESSURE_PIN, INPUT);
}

void GetPressure()
{
  String pressure = "Static Pressure,";
  //pressure += timeString;
 //pressure += "0";
 // pressure += ",";
 // int i = analogRead(A0);//PRESSURE_PIN);
  static_pressure = analogRead(PRESSURE_PIN) * (5.0 / 1023.0) * 5;
  filterValue(static_pressure, filteredValues.filteredSP, sp_fc);
  pressure += filteredValues.filteredSP;
  pressure += ",in. w.";
  Serial.println(pressure);
}
