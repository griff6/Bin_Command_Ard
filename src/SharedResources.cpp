#include "SharedResources.h"
#include "FlashStorage.h"
#include <SD.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include "wirelessConnection.h"
#include "sdOperations.h"
#include "Engine_Control.h"
#include "binCables.h"

FlashStorage(config_flash, Config);

unsigned long ledFlashInterval = 1000;  //delay between attempts to start engine
unsigned long prevLEDflash = 0;
int LEDstate = HIGH;

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

  //publishDataMessage();
  ProcessDataRecord();
}

void GetBatteryVoltage()
{
  //get the battery voltage
  analogReadResolution(12);
  batteryVoltage = analogRead(BATT_PIN) * (3.3 / 4096);
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

void AutoControl()
{
  if(userEngineCommand != AUTO)
  {
    return;
  }

  if(fanMode == DRY)
  {
    AutoDry();
  }else if(fanMode == AERATE)
  {
    AutoAerate();
  }
}

void AutoDry()
{
  if(filteredValues.filteredAirTemp < config.minDryingTemperature)
  {
    //No point running the fan if it is too cold.
    if(engineState == RUNNING)
      SetThrottle(COOLDOWN);

    return;
  }

  if(avgGrainMoisture <= config.targetMoisture)
  {
    AutoAerate();
    return;
  }else{
    if(engineState == RUNNING)
      return;

    startEngine = true;
    starterAttempt = 0;
    EngineStartSequence();
  }
}

void AutoAerate()
{
  double emc;
  if(maxGrainTemp < config.targetTemperature+5)
  {
    if(engineState == RUNNING)
      SetThrottle(COOLDOWN);

    return;
  }

  emc = CalculateMoisture(filteredValues.filteredAirHumidity, filteredValues.filteredAirTemp);

  if(emc > config.targetMoisture)
  {
    if(engineState == RUNNING)
      SetThrottle(COOLDOWN);
  }else{
    if(engineState == RUNNING)
      return;

    startEngine = true;
    starterAttempt = 0;
    EngineStartSequence();
  }
}

void CheckConnectionMode()
{
  if(bluetoothConnected)
  {
    ledFlashInterval = 1000;
  }
  else if(WirelessConnected() && MQTTConnected())
  {
    digitalWrite(CONN_LED_PIN, HIGH);
    LEDstate = HIGH;
    return;
  }else if(WirelessConnected() && !MQTTConnected())
  {
    ledFlashInterval = 100;
  }else{
    //Serial.println("Not Connected");
    digitalWrite(CONN_LED_PIN, LOW);
    LEDstate = LOW;
    return;
  }

  if (millis() - prevLEDflash >= ledFlashInterval)
  {
    prevLEDflash = millis();

    if(LEDstate == HIGH)
    {
      LEDstate = LOW;
      digitalWrite(CONN_LED_PIN, LOW);
    }else{
      LEDstate = HIGH;
      digitalWrite(CONN_LED_PIN, HIGH);
    }
  }
}

String print2digits(int number) {
  String ret = "";
  ret = ret + number;

  if (number < 10) {
    //Serial.print("0"); // print a 0 before if the number is < than 10
    ret = "0" + ret;
  }
  //Serial.print(number);
  return ret;
}

//round a number to one decimal place
float Round(float var)
{
  float value = (int)(var * 10 + 0.5);
  return (float)value/10;
}

void ProcessDataRecord()
{
  int fanStatus = 0;
  String cableNum = "C0";

  if(filteredValues.filteredRPM > 100)
  {
    fanStatus = 1;
  }

  Serial.println();
  Serial.println("Publishing message");


  //https://arduinojson.org/v6/assistant/ gives some details on how to calculate the size
  //https://arduinojson.org/v5/assistant/
  //https://arduinojson.org/v5/faq/how-to-determine-the-buffer-size/
  //const size_t capacity = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5);
  //const size_t capacity;// = JSON_OBJECT_SIZE(20);// + JSON_OBJECT_SIZE(16*numCables);
  //DynamicJsonDocument doc(capacity);
  //DynamicJsonDocument doc(431);
  //size_t capacity = JSON_OBJECT_SIZE(78);

  String ts = "";
  ts = ts + print2digits(rtc.getYear());
  ts += "-";
  ts += print2digits(rtc.getMonth());
  ts += "-";
  ts += print2digits(rtc.getDay());
  ts += " ";
  ts += print2digits(rtc.getHours());
  ts += ":";
  ts += print2digits(rtc.getMinutes());

  //StaticJsonDocument<capacity> doc;
  //size_t capacity = JSON_OBJECT_SIZE(28);
  size_t capacity = JSON_OBJECT_SIZE(67);
  DynamicJsonDocument doc(capacity);

  doc["ts"] = ts;
  doc["msg"] = 0;
  doc["ap"] = Round(filteredValues.filteredAirPressure);
  doc["ah"] = Round(filteredValues.filteredAirHumidity);
  doc["at"] = Round(filteredValues.filteredAirTemp);
  doc["fs"] = fanStatus;
  doc["fh"] = Round(filteredValues.filteredFanHumidity);
  doc["ft"] = Round(filteredValues.filteredFanTemp);
  doc["sp"] = Round(filteredValues.filteredSP);//serialized(String(filteredValues.filteredSP,1));
  doc["minT"] = Round(minGrainTemp);
  doc["maxT"] = Round(maxGrainTemp);
  doc["avT"] = Round(avgGrainTemp);
  doc["minM"] = Round(minGrainMoisture);
  doc["maxM"] = Round(maxGrainMoisture);
  doc["avM"] = Round(avgGrainMoisture);
  doc["mode"] = fanMode;
  doc["bn"] = config.batchNumber;
  doc["ber"] = Round(config.batchEngineTime);
  doc["bat"] = Round(config.batchAerateTime);
  doc["bdt"] = Round(config.batchDryingTime);
  doc["gn"] = config.grain;

  publishDataMessage(doc, false);     //sdOperations
  doc["timestamp"] = rtc.getEpoch();
  doc["batchStart"] = config.batchStartTime;
  SaveDataRecord(doc);                //wirelessConnection

  for(int cable = 0; cable < numCables; cable++)
  {
    DynamicJsonDocument doc(capacity);

    doc["ts"] = ts;
    doc["msg"] = cable+1;

    JsonArray cableTemp = doc.createNestedArray("ct");

    for(int i = 0; i < cableValues[cable].numSensors; i++)
    {
      cableTemp.add(Round(cableValues[cable].sensors[i].temperature));
    }

    JsonArray cableMois = doc.createNestedArray("mois");

    for(int i = 0; i < cableValues[cable].numSensors; i++)
    {
      cableMois.add(Round(cableValues[cable].sensors[i].moisture));
    }

    SaveDataRecord(doc);            //sdOperations
    publishDataMessage(doc, true);  //wirelessConnection

    Serial.println();
  }

}
