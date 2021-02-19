#include "sdOperations.h"
#include <SD.h>
#include <ArduinoJson.h>
#include "SharedResources.h"
#include <TimeLib.h>
#include "time.h"

const char *configFileName = "config.txt";

void InitiallizeSD()
{
  Serial.print("Initializing SD card...");

  //float tempEngineTime = engineTime_Flash.read();

  if (!SD.begin(A0)) {
    Serial.println("Could not initiallize SD card");
    //while (1);
  }else{
    Serial.println("SD card initiallized.");

    if(!SD.exists(configFileName))
    {
      Serial.println("Configuration file does not exist, creating it now");
      //File file = SD.open(configFileName, FILE_WRITE);
      //file.close();

      config = GetFlashConfiguration();//config_flash.read();

      SaveConfigFile();

    }else{

      GetConfigFile();
      Config tempConfig = GetFlashConfiguration();//config_flash.read();

      if(tempConfig.engineTime <= config.engineTime)
      {
        Serial.println("Using configuration file values");

        //config_flash.write(config);
        SaveFlashConfiguration();
      }else{
        Serial.println("Using flash memory configuration values");

        config.engineTime = tempConfig.engineTime;
        config.batchAerateTime = tempConfig.batchAerateTime;
        config.batchDryingTime = tempConfig.batchDryingTime;
        config.batchNumber = tempConfig.batchNumber;
        config.batchStartTime = tempConfig.batchStartTime;
        memcpy(config.grain, tempConfig.grain, sizeof(tempConfig.grain));
      }
    }
  }
}

void GetConfigFile()
{
  Serial.println("Reading Configuration File");
  File file = SD.open(configFileName);

  const size_t capacity = JSON_OBJECT_SIZE(34);
  StaticJsonDocument<capacity> doc;

  DeserializationError error = deserializeJson(doc, file);
  if(error)
    Serial.print("Failed to read from the configuration file");

  strlcpy(config.grain, doc["grain"], sizeof(config.grain));
  config.batchNumber = doc["batchNumber"];
  config.engineTime = doc["engineTime"];
  config.batchEngineTime = doc["batchEngineTime"];
  config.batchDryingTime = doc["batchDryingTime"];
  config.batchAerateTime = doc["batchAerateTime"];
  config.batchStartTime = doc["batchStartTime"];

  //Serial.print("Grain in GetConfigFile: ");
  //Serial.println(config.grain);
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
  doc["batchStartTime"] = config.batchStartTime;

  if(serializeJson(doc, file) == 0)
  {
    Serial.println("Failed to write the configuration file");
  }

  //serializeJsonPretty(doc, Serial);
  //Serial.println();

  file.close();

  //config_flash.write(config);
  SaveFlashConfiguration();
}

void SaveDataRecord(DynamicJsonDocument doc)
{
  char dataFile[25];
  String date = "";
  date += 2000+rtc.getYear();
  //date += "-";

  if(rtc.getMonth() < 10)
  {
    date += "0";
  }
  date += rtc.getMonth();


//date += "-";

  if(rtc.getDay() < 10)
    date += "0";

  date += rtc.getDay();

  date += ".txt";

  date.toCharArray(dataFile, 25);

  Serial.print("Writing to data file: ");
  Serial.println(dataFile);

  File file = SD.open(date, FILE_WRITE);
  if(!file){
    Serial.println("Failed to create open or create the data file");
    return;
  }

  if(serializeJson(doc, file) == 0)
  {
    Serial.print("Failed to write data to file - ");
    Serial.println(dataFile);
  }

  serializeJsonPretty(doc, Serial);
  Serial.println();

  file.println();

  file.close();

//For Debugging only
  GetDataRecord(1613772391);
}

void GetDataRecord(unsigned long input)
{
  const size_t capacity = JSON_OBJECT_SIZE(1024);
  time_t time = input;
  String dateString = "";
  dateString += year(time);
  if(month(time) < 10)
    dateString += "0";
  dateString += month(time);
  if(day(time) < 10)
    dateString += "0";
  dateString += day(time);

  dateString += ".txt";

  Serial.print("Opening Data File: ");
  Serial.println(dateString);

  char dataFile[25];
  dateString.toCharArray(dataFile, 20);

  File file = SD.open(dataFile, FILE_READ);
  if(!file)
  {
    Serial.print("Failed to open data file: ");
    Serial.println(dateString);
  }

  String line;
  while(file.available()){
    line = file.readStringUntil('}');
    Serial.println(line);
    StaticJsonDocument<capacity> doc;

    DeserializationError error = deserializeJson(doc, file);
    if(error)
      Serial.print("Failed to deserialize data from data file");

    //Get whatever data is required
    Serial.print("TimeStamp: ");
    String ts = doc["ts"];
    Serial.println(ts);
  }

  file.close();
}
