#include "manageConnections.h"
#include "gsmConnection.h"
#include "SharedResources.h"

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
    if(updateEngineState)
    {
      publishEngineState();
    }
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
