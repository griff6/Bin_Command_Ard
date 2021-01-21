#include "manageConnections.h"
#include "gsmConnection.h"

bool gsmConnected = false;

void InitiallizeConnections()
{
  gsmConnected = connectGSM();

  if(gsmConnected)
    initiallizeMQTT();
}

void MaintainConnections()
{
  if(gsmConnected){
    //Serial.println(".");
    MQTT_Poll();
    //RequestTimeStamp();
  //  GetLocation();
  }
}

void GetGSMLocation()
{
  GetLocation();
}

void HandleHourlyData()
{
  publishDataMessage();
}
