#include "SharedResources.h"
#include "FlashStorage.h"
#include <SD.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include "gsmConnection.h"
#include "sdOperations.h"

FlashStorage(config_flash, Config);

//Filter the raw values
void filterValue(float newReading, float &filtValue, float fc)
{
  if(newReading == 0 || filtValue >= 65536)
    {
      filtValue = newReading;
    }else
    {
      filtValue = (1-fc)*filtValue + fc * newReading;
    }
}

void SetRTC_Alarm()
{
  uint8_t hour = rtc.getHours();
  //uint8_t minutes = rtc.getMinutes();

  //Serial.println("Setting RTC");
  //Serial.print(hour);
  //Serial.print(":");
  //Serial.println(minutes);

  if(hour >= 23)
    hour = 0;
  else
    hour++;

  rtc.setAlarmTime(hour, 0, 0);
}

void RTC_Alarm()
{
  hourlyData = true;
}

void StartNewBatch(const char *grainName)
{
  strlcpy(config.grain, grainName, sizeof(config.grain));
  config.batchNumber++;
  config.batchEngineTime = 0;
  config.batchDryingTime = 0;
  config.batchAerateTime = 0;
  config.batchStartTime = rtc.getEpoch();

  config_flash.write(config);

  SaveConfigFile();

  publishDataMessage();
}

void GetBatteryVoltage()
{
  //get the battery voltage
  analogReadResolution(12);
  batteryVoltage = analogRead(A3) * (3.3 / 4096);
  batteryVoltage = 10.652 * pow(batteryVoltage,2)-29.608 * batteryVoltage + 27.713;
}

Config GetFlashConfiguration()
{
  return config_flash.read();
}

void SaveFlashConfiguration()
{
  config_flash.write(config);
}
