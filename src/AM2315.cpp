#include "AM2315.h"
#include "SharedResources.h"

Adafruit_AM2315 am2315;     //library for the Ambient temperature and humidity sensor
float temperature;
float humidity;
int count = 0;
float fan_fc = .75;//.2;            // higher value puts less filtering


bool StartAM2315()
{
  //start am2315 (fan) sensor
  am2315.begin();
  delay(2000);

  return true;
}

void GetFanData()
{

  count = 0;
  temperature = 65536;
  humidity = 65536;

  while(count <= 100 && !am2315.readTemperatureAndHumidity(&temperature, &humidity)){
    count++;
    delay(20);
  }

  filterValue(temperature, filteredValues.filteredFanTemp, fan_fc);
  filterValue(humidity, filteredValues.filteredFanHumidity, fan_fc);

  Serial.print("Fan Temperature: ");
  Serial.println(filteredValues.filteredFanTemp);
  //Serial.print("Fan Humidity: ");
  //Serial.println(filteredValues.filteredFanHumidity);
}
