#include <Arduino.h>
#include "main.h"
#include "AM2315.h"
#include "SharedResources.h"
#include "BME280.h"
#include "Bin_Pressure_Sensor.h"
#include "Engine_Control.h"
#include "RPM_Sensor.h"
#include "wirelessConnection.h"
#include "binCables.h"
#include "TimedAction.h"
#include "sdOperations.h"
#include "bluetooth.h"

FilteredValues filteredValues;
Config config;
CableValues cableValues[3];
EngineCommand userEngineCommand = OFF;
EngineState engineState = STOPPED;
FanMode fanMode = DRY;
ConnectionType connectionType = NONE;
bool startEngine = false;
int starterAttempt = 500;
bool updateCloudEngineState = false;
bool updateBLEengineState = false;
bool hourlyData;
RTCZero rtc;
float maxGrainTemp = -65536;
float minGrainTemp = 65536;
float avgGrainTemp = 65536;
float maxGrainMoisture = -65536;
float minGrainMoisture = 65536;
float avgGrainMoisture = 65536;
int numCables = 0;
bool timeIsSet = false;
float batteryVoltage = 0;
float minDryingTemperature = -10;
bool bluetoothConnected = false;

TimedAction collectSensorData = TimedAction(300000, CollectSensorData);     //Get sensor data every 5 minutes
TimedAction collectBinSensorData = TimedAction(900001, GetBinCableValues);   //Get the cable data every 15 minutes
TimedAction engineTimer = TimedAction(60002, UpdateEngineTimer);
TimedAction saveConfiguration = TimedAction(60003, SaveConfigFile);
long rpmCalcInterval = 1000;    //how often the program will calculate RPM
long printInterval = 60000;//60000;   //how often the program will print the data out (60000 is 1 minute)
long configSaveInterval = 60000;
long engineTimerInterval = 60000;
long prevRPMCalc = 0;
long prevPrint = 0;
long prevConfigSave = 0;
long lastEngineTime = 0;
float updateTime;


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

  pinMode(CONN_LED_PIN, OUTPUT);

  InitiallizeSD();

  Wire.begin();

  //InitiallizeConnections();
  ConnectWirelessNetwork();
  //CheckConnectionMode();

  SetupRPMSensor();
  SetupPressure();

  //Setup BME (ambient) sensor
  SetupBME280();

  StartAM2315();

  SetupBinCables();

  InitiallizeBluetooth();

  GetBinCableValues();
  GetBatteryVoltage();
  Save_RPM();
  GetPressure();
  PrintBME280Data();
  GetFanData();

  lastEngineTime = millis();

  Serial.println("Program Initiallized");
}

void loop() {
  long currentMillis = millis();
  long diffRPMTime = currentMillis - prevRPMCalc;

  CheckConnectionMode(); //SharedResources
  MaintainConnections();

  CheckBluetooth();

  collectSensorData.check();
  collectBinSensorData.check();
  engineTimer.check();
  saveConfiguration.check();

  //Calculate a new RPM value
  if(diffRPMTime >= rpmCalcInterval)
  {
    Calculate_RPM(diffRPMTime);
    prevRPMCalc = currentMillis;
  }

  if(hourlyData){
    //HandleHourlyData();
    ProcessDataRecord();
    SetRTC_Alarm();
    hourlyData = false;
  }
  EngineController();
}

void CollectSensorData()
{
  Save_RPM();

  GetPressure();
  PrintBME280Data();
  GetFanData();

  GetBatteryVoltage();

  //GetGSMLocation();
  Serial.print(".");
}

void UpdateEngineTimer()
{
  if(engineState == RUNNING)
  {
  //  long temp = millis();
    long tempTime = millis() - lastEngineTime;
    float update = tempTime;

    updateTime = update / 3600000; //convert the time from ms to hours

    Serial.print("   updateTime: ");
    Serial.println(updateTime);

    config.engineTime += updateTime;
    config.batchEngineTime += updateTime;

    if(fanMode == AERATE)
    {
      config.batchAerateTime += updateTime;
    }if(fanMode == DRY){
      config.batchDryingTime += updateTime;
    }
  }
}
