#include "SharedResources.h"
#include "FlashStorage.h"
#include <SD.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include "gsmConnection.h"

FlashStorage(grainType_Flash, String);
FlashStorage(engineTime_Flash, float);
FlashStorage(batchEngineTime_Flash, float);
FlashStorage(batchDryingTime_Flash, float);
FlashStorage(batchAerationTime_Flash, float);
FlashStorage(batchNumber_Flash, int);

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

void InitiallizeSD()
{
  Serial.print("Initializing SD card...");

  float tempEngineTime = engineTime_Flash.read();

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

      config.engineTime = tempEngineTime;
      config.batchEngineTime = batchEngineTime_Flash.read();
      config.batchDryingTime = batchDryingTime_Flash.read();
      config.batchAerateTime = batchAerationTime_Flash.read();
      config.batchNumber = batchNumber_Flash.read();
      grainType_Flash.read().toCharArray(config.grain, sizeof(config.grain));

      SaveConfigFile();

    }else{

      GetConfigFile();

      grainType_Flash.write(config.grain);

      if(tempEngineTime < config.engineTime)
      {
        Serial.println("Using configuration file values");
        engineTime_Flash.write(config.engineTime);
        batchEngineTime_Flash.write(config.batchEngineTime);
        batchDryingTime_Flash.write(config.batchDryingTime);
        batchAerationTime_Flash.write(config.batchAerateTime);
        batchNumber_Flash.write(config.batchNumber);
        grainType_Flash.write(config.grain);
      }else{
        Serial.println("Using flash memory configuration values");
        config.engineTime = tempEngineTime;
        config.batchEngineTime = batchEngineTime_Flash.read();
        config.batchDryingTime = batchDryingTime_Flash.read();
        config.batchAerateTime = batchAerationTime_Flash.read();
        config.batchNumber = batchNumber_Flash.read();
        grainType_Flash.read().toCharArray(config.grain, sizeof(config.grain));
      }
    }
  }
}

void GetConfigFile()
{
  Serial.println("Reading Configuration File");
  File file = SD.open(configFileName);

  const size_t capacity = JSON_OBJECT_SIZE(30);
  StaticJsonDocument<capacity> doc;

  DeserializationError error = deserializeJson(doc, file);
  if(error)
    Serial.print("Failed to read from the configuration file");

  strlcpy(config.grain, doc["grain"] | "", sizeof(config.grain));
  config.batchNumber = doc["batchNumber"];
  config.engineTime = doc["engineTime"];
  config.batchEngineTime = doc["batchEngineTime"];
  config.batchDryingTime = doc["batchDryingTime"];
  config.batchAerateTime = doc["batchAerateTime"];

  //serializeJsonPretty(doc, Serial);
  //Serial.println();

  file.close();
}

void SaveConfigFile()
{
  //Serial.println("Saving Configuration");
  SD.remove(configFileName);

  File file = SD.open(configFileName, FILE_WRITE);
  if(!file){
    Serial.println("Failed to create the configuration file");
    return;
  }

  const size_t capacity = JSON_OBJECT_SIZE(30);
  StaticJsonDocument<capacity> doc;

  doc["grain"] = config.grain;
  doc["batchNumber"] = config.batchNumber;
  doc["engineTime"] = config.engineTime;
  doc["batchEngineTime"] = config.batchEngineTime;
  doc["batchDryingTime"] = config.batchDryingTime;
  doc["batchAerateTime"] = config.batchAerateTime;

  if(serializeJson(doc, file) == 0)
  {
    Serial.println("Failed to write the configuration file");
  }

  //serializeJsonPretty(doc, Serial);
  //Serial.println();

  file.close();

  engineTime_Flash.write(config.engineTime);
  batchEngineTime_Flash.write(config.batchEngineTime);
  batchDryingTime_Flash.write(config.batchDryingTime);
  batchAerationTime_Flash.write(config.batchAerateTime);
  batchNumber_Flash.write(config.batchNumber);
  grainType_Flash.write(config.grain);
}

void StartNewBatch(const char *grainName)
{
  strlcpy(config.grain, grainName, sizeof(config.grain));
  config.batchNumber++;
  config.batchEngineTime = 0;
  config.batchDryingTime = 0;
  config.batchAerateTime = 0;

  engineTime_Flash.write(config.engineTime);
  batchEngineTime_Flash.write(config.batchEngineTime);
  batchDryingTime_Flash.write(config.batchDryingTime);
  batchAerationTime_Flash.write(config.batchAerateTime);
  batchNumber_Flash.write(config.batchNumber);
  grainType_Flash.write(config.grain);

  SaveConfigFile();

  publishDataMessage();
}
