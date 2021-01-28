#include "SharedResources.h"
#include "FlashStorage.h"
#include <SD.h>
#include <SPI.h>
#include <ArduinoJson.h>

FlashStorage(grainType_Flash, String);
FlashStorage(engineTime_Flash, unsigned long);
FlashStorage(batchEngineTime_Flash, unsigned long);
FlashStorage(batchDryingTime_Flash, unsigned long);
FlashStorage(batchAerationTime_Flash, unsigned long);

const char *configFileName = "config.txt";

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
  uint8_t minutes = rtc.getMinutes();

  //Serial.println("Setting RTC");
  Serial.print(hour);
  Serial.print(":");
  Serial.println(minutes);

  rtc.setAlarmTime(hour+1, 0, 0);
}

void RTC_Alarm()
{
  hourlyData = true;
}

void ReadFlashValues()
{

}

void InitiallizeSD()
{
  Serial.print("Initializing SD card...");

  unsigned long tempEngineTime = engineTime_Flash.read();
  unsigned long tempBatchEngineTime = batchEngineTime_Flash.read();
  unsigned long tempBatchDryingTime = batchDryingTime_Flash.read();
  unsigned long tempBatchAerationTime = batchAerationTime_Flash.read();

  if (!SD.begin(0)) {
    Serial.println("Could not initiallize SD card");
    //while (1);
  }else{
    Serial.println("SD card initiallized.");

    if(!SD.exists(configFileName))
    {
      Serial.println("Configuration file does not exist, creating it now");
      File file = SD.open(configFileName, FILE_WRITE);
      file.close();

      if(tempEngineTime >= 0)
        config.engineTime = tempEngineTime;
      else{
          engineTime_Flash.write(0);
          config.engineTime = 0;
      }

      if(tempBatchEngineTime >= 0)
      {
        config.batchEngineTime = tempBatchEngineTime;
      }else{
        batchEngineTime_Flash.write(0);
        config.batchEngineTime = 0;
      }

      if(tempBatchDryingTime >= 0)
      {
        config.batchDryingTime = tempBatchDryingTime;
      }else{
        batchDryingTime_Flash.write(0);
        config.batchDryingTime = 0;
      }

      if(tempBatchAerationTime >= 0)
      {
        config.batchAerateTime = tempBatchAerationTime;
      }else{
        batchAerationTime_Flash.write(0);
        config.batchAerateTime = 0;
      }

      grainType_Flash.read().toCharArray(config.grain, sizeof(config.grain));

      SaveConfigFile();

    }else{

      GetConfigFile();

      grainType_Flash.write(config.grain);

      if(tempEngineTime > config.engineTime)
      {
        engineTime_Flash.write(config.engineTime);
      }else{
        config.engineTime = tempEngineTime;
      }

      if(tempBatchEngineTime > config.batchEngineTime){
        batchEngineTime_Flash.write(config.batchEngineTime);
      }else{
        config.batchEngineTime = tempBatchEngineTime;
      }

      if(tempBatchDryingTime > config.batchDryingTime){
        batchDryingTime_Flash.write(config.batchDryingTime);
      }else{
        config.batchDryingTime = tempBatchDryingTime;
      }

      if(tempBatchAerationTime > config.batchAerateTime){
        batchAerationTime_Flash.write(config.batchAerateTime);
      }else{
        config.batchAerateTime = tempBatchAerationTime;
      }
    }
  }
}

void GetConfigFile()
{
  Serial.println("Reading Configuration File");
  File file = SD.open(configFileName);

  const size_t capacity = JSON_OBJECT_SIZE(12);
  StaticJsonDocument<capacity> doc;

  DeserializationError error = deserializeJson(doc, file);
  if(error)
    Serial.print("Failed to read from the configuration file");

  strlcpy(config.grain, doc["grain"] | "", sizeof(config.grain));
  config.engineTime = doc["engineTime"] | 0;
  config.batchEngineTime = doc["batchEngineTime"] | 0;
  config.batchDryingTime = doc["batchDryingTime"] | 0;
  config.batchAerateTime = doc["batchAerateTime"] | 0;

  serializeJsonPretty(doc, Serial);
  Serial.println();

  file.close();
}

void SaveConfigFile()
{
  Serial.println("Saving Configuration");
  SD.remove(configFileName);

  File file = SD.open(configFileName, FILE_WRITE);
  if(!file){
    Serial.println("Failed to create the configuration file");
    return;
  }

  const size_t capacity = JSON_OBJECT_SIZE(6);
  StaticJsonDocument<capacity> doc;

  doc["grain"] = config.grain;
  doc["engineTime"] = config.engineTime;
  doc["batchEngineTime"] = config.batchEngineTime;
  doc["batchDryingTime"] = config.batchDryingTime;
  doc["batchAerateTime"] = config.batchAerateTime;

  if(serializeJson(doc, file) == 0)
  {
    Serial.println("Failed to write the configuration file");
  }

  serializeJsonPretty(doc, Serial);
  Serial.println();

  file.close();

  grainType_Flash.write(config.grain);
  engineTime_Flash.write(config.engineTime);
  batchEngineTime_Flash.write(config.batchEngineTime);
  batchDryingTime_Flash.write(config.batchDryingTime);
  batchAerationTime_Flash.write(config.batchAerateTime);
}

void StartNewBatch(const char *grainName)
{
  strlcpy(config.grain, grainName, sizeof(config.grain));
}
