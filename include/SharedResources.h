#include <Arduino.h>
#include <RTCZero.h>

//pin definitions
#define CS_PIN        0//A0  //chip select pin for SD card (A0 on production)
#define PRESSURE_PIN  A1  //Input for static pressure sensor
#define AIRFLOW_PIN   A2  //Input for airflow sensor
#define BATT_PIN      A3  //Input pin for battery sensing
#define CONN_LED_PIN  A4  //Pin for connection LED
#define ACC_PIN       2   //Pin for the Accessories output signal (A5 on production)
#define START_PIN     3   //Pin for the starter Output signal     (A6 on production)
//#define SHT10_DATA    0   //Pin for SHT10 (fan temperature/humidity) sensor
#define SHT10_CLK     1   //Pin for SHT10 (fan temperature/humidity) sensor
//#define THT_UP        2   //Pin to drive the actuator for throttle up
//#define THT_DN        3   //Pin to drive the actuator for throttle down
#define BIN_CBL_1     4   //Pin for bin cable 1
#define RPM_PIN       5   //Pin for the RPM Input
#define BIN_CBL_2     6   //Pin for bin cable 2
#define BIN_CBL_3     7   //Pin for bin cable 3


struct FilteredValues{
public:
  float filteredRPM = 0;
  float filteredSP = 0;
  float filteredAirTemp = 0;
  float filteredAirHumidity = 0;
  float filteredAirPressure = 0;
  float filteredFanTemp = 0;
  float filteredFanHumidity = 0;
};

struct BinSensor{
public:
  int address;
  float temperature;
  double moisture;
  float humidity;
};

class CableValues
{
public:

  float avgT = 0;
  float avgM = 0;
  int maxAddress = -1;
  int minAddress = 65536;
  int numSensors = 0;
  BinSensor sensors[10];
};

enum EngineCommand{
  OFF,
  ON,
  AUTO
};

enum EngineState{
  STOPPED,
  STARTING,
  RUNNING,
  WARMUP,
  COOLDOWN,
  FAILED_START,
};

enum FanMode{
  AERATE,
  DRY,
};

enum ConnectionType{
  NONE,
  BT,
  WIRELESS,
};

struct Config{
  char grain[30];
  int batchNumber;
  float engineTime;
  float batchEngineTime;
  float batchDryingTime;
  float batchAerateTime;
  unsigned long batchStartTime;
  float minDryingTemperature;
  float targetMoisture;
  float targetTemperature;
  char binName[100];

};

extern float maxGrainTemp;
extern float minGrainTemp;
extern float avgGrainTemp;
extern float maxGrainMoisture;
extern float minGrainMoisture;
extern float avgGrainMoisture;
extern int numCables;

extern FilteredValues filteredValues;
extern CableValues cableValues[3];
extern EngineCommand userEngineCommand;
extern EngineState engineState;
extern FanMode fanMode;
extern bool startEngine;
extern int starterAttempt;
extern bool updateEngineState;
extern Config config;
extern bool timeIsSet;
extern float batteryVoltage;
extern float minDryingTemperature;

extern RTCZero rtc;
extern bool hourlyData;

extern ConnectionType connectionType;



void filterValue(float newReading, float &filtValue, float fc);
void SetRTC_Time(uint32_t unixTime);
void SetRTC_Alarm();
void RTC_Alarm();
void SetEngineTimerAlarm();
void EngineTimerAlarm();
void StartNewBatch(const char *grainName);
void GetBatteryVoltage();
Config GetFlashConfiguration();
void SaveFlashConfiguration();
void AutoControl();
void AutoDry();
void AutoAerate();
void CheckConnectionMode();
