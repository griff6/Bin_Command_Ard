#include "SharedResources.h"

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

  rtc.setAlarmTime(hour+1, minutes, 0);
}

void RTC_Alarm()
{
  hourlyData = true;
}
