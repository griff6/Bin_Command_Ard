#include <Arduino.h>

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

extern FilteredValues filteredValues;
extern int accState;

void filterValue(float newReading, float &filtValue, float fc);
