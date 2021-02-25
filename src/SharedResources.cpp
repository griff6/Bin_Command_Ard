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

  publishDataMessage();
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
  if(WirelessConnected() && MQTTConnected())
  {
    //Serial.println("Connected");
    digitalWrite(CONN_LED_PIN, HIGH);
    LEDstate = HIGH;
  }else if(WirelessConnected() && !MQTTConnected())
  {
    ledFlashInterval = 100;

    if (millis() - prevLEDflash >= ledFlashInterval)
    {
      prevLEDflash = millis();

      //Serial.println("Connected - no MQTT");

      if(LEDstate == HIGH)
      {
        LEDstate = LOW;
        digitalWrite(CONN_LED_PIN, LOW);
      }else{
        LEDstate = HIGH;
        digitalWrite(CONN_LED_PIN, HIGH);
      }
    }
  }else{
    //Serial.println("Not Connected");
    digitalWrite(CONN_LED_PIN, LOW);
    LEDstate = LOW;
  }
}
