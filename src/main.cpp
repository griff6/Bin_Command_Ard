#include <Arduino.h>
#include "AM2315.h"
#include "SharedResources.h"
#include "BME280.h"
#include "Bin_Pressure_Sensor.h"
#include "Engine_Control.h"
#include "RPM_Sensor.h"
#include "Connection.h"



FilteredValues filteredValues;
int accState;

long mqttInterval = 500;    //Poll the mqtt every 500 ms
long rpmCalcInterval = 1000;    //how often the program will calculate RPM
long printInterval = 10000;//60000;   //how often the program will print the data out (60000 is 1 minute)
long dataChannelInterval = 60000;//1800000;//3600000;    //Update Every Hour
long engineTimeInterval = 60000;   //update every minute
long prevMqttCalc = 0;
long prevRPMCalc = 0;
long prevPrint = 0;
long prevEngineTime = 0;
long prevDataChannel = 0;

bool firstLoop = true;

void setup() {
  Serial.begin(9600);
  // Wait for 20 seconds to see if Serial is connected, otherwise run the Sktech without Serial debugging
  int i;
  for(i=0; i<20; i++){
    if(Serial) break;
    delay(1000);
  }

  Serial.println("Starting the Program");
  accState = 0;
  Wire.begin();

  initiallizeMQTT();

  SetupRPMSensor();
  SetupPressure();

  //Setup BME (ambient) sensor
  SetupBME280();

  StartAM2315();

  Serial.println("Program Initiallized");
}

void loop() {
  long currentMillis = millis();
  long diffRPMTime = currentMillis - prevRPMCalc;

  //if(currentMillis - prevMqttCalc >= mqttInterval){
    MQTT_Poll();
  //  prevMqttCalc = currentMillis;
  //}

  //Calculate a new RPM value
  if(diffRPMTime >= rpmCalcInterval)
  {
    Calculate_RPM(diffRPMTime);

    prevRPMCalc = currentMillis;
  }

  //This is to collect the data and print it to the serial port
  if(currentMillis - prevPrint >= printInterval || firstLoop == true)
  {
    Save_RPM();

    GetPressure();
    PrintBME280Data();
    GetFanData();
    Serial.println();

    prevPrint = currentMillis;
  }
}
