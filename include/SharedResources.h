#include <Arduino.h>
#include <RTCZero.h>

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
  FAILED_START,
};

enum FanMode{
  AERATE,
  DRY,
};

struct Config{
  char grain[30];
  int batchNumber;
  float engineTime;
  float batchEngineTime;
  float batchDryingTime;
  float batchAerateTime;
};

extern float maxTemp;
extern float minTemp;
extern float avgTemp;
extern float maxMoisture;
extern float minMoisture;
extern float avgMoisture;
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
extern bool timeSet;

extern RTCZero rtc;
extern bool hourlyData;

void filterValue(float newReading, float &filtValue, float fc);
void SetRTC_Time(uint32_t unixTime);
void SetRTC_Alarm();
void RTC_Alarm();
void SetEngineTimerAlarm();
void EngineTimerAlarm();
void InitiallizeSD();
void GetConfigFile();
void SaveConfigFile();
void StartNewBatch(const char *grainName);
