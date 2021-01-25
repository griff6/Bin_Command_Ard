#include <Arduino.h>
#include <RTCZero.h>

class FilteredValues{
public:
  float filteredRPM = 65536;
  float filteredSP = 65536;
  float filteredAirTemp = 65536;
  float filteredAirHumidity = 65536;
  float filteredAirPressure = 65536;
  float filteredFanTemp = 65536;
  float filteredFanHumidity = 65536;
  float engineTime = 0;
};

class CableValues
{
public:
  float s1T = 65536;
  float s1M = 65536;
  float s2T = 65536;
  float s2M = 65536;
  float s3T = 65536;
  float s3M = 65536;
  float s4T = 65536;
  float s4M = 65536;
  float s5T = 65536;
  float s5M = 65536;
  float s6T = 65536;
  float s6M = 65536;
  float s7T = 65536;
  float s7M = 65536;
  float s8T = 65536;
  float s8M = 65536;
  float avgT = 65536;
  float avgM = 65536;
};

enum EngineControl{
  OFF,
  ON,
  AUTO
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
extern int accState;
extern int currentBatch;
extern EngineControl engineControl;

extern RTCZero rtc;
extern bool hourlyData;

void filterValue(float newReading, float &filtValue, float fc);
void SetRTC_Time(uint32_t unixTime);
void SetRTC_Alarm();
void RTC_Alarm();
