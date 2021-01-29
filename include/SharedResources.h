#include <Arduino.h>
#include <RTCZero.h>

class FilteredValues{
public:
  float filteredRPM = 0;
  float filteredSP = 0;
  float filteredAirTemp = 0;
  float filteredAirHumidity = 0;
  float filteredAirPressure = 0;
  float filteredFanTemp = 0;
  float filteredFanHumidity = 0;
};

class CableValues
{
public:
  float s1T = 0;
  float s1M = 0;
  float s2T = 0;
  float s2M = 0;
  float s3T = 0;
  float s3M = 0;
  float s4T = 0;
  float s4M = 0;
  float s5T = 0;
  float s5M = 0;
  float s6T = 0;
  float s6M = 0;
  float s7T = 0;
  float s7M = 0;
  float s8T = 0;
  float s8M = 0;
  float avgT = 0;
  float avgM = 0;
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

extern FilteredValues filteredValues;
extern CableValues cable1;
extern CableValues cable2;
extern CableValues cable3;
//extern int currentBatch;
extern EngineCommand userEngineCommand;
extern EngineState engineState;
extern FanMode fanMode;
extern bool startEngine;
extern int starterAttempt;
extern bool updateEngineState;
extern Config config;

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
