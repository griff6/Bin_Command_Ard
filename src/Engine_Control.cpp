#include "Engine_Control.h"
#include "SharedResources.h"

#define START_PIN 2     //Pin for the start output relay
#define ACC_PIN 1       //Pin for the Accessories relay

const int STARTER_TIME = 1250;      //time to run the starter in ms
long engineStartInterval = 10000;  //delay between attempts to start engine
long prevStartAttempt = 0;
int starterSequence = 0;        //Used to store if the engine should be starting or not
int starterAttempt = 0;

void EngineStartSequence()
{
  if (!starterSequence || accState == 0)
  {
    starterSequence = false;
    return;
  }

  unsigned long currentMillis = millis();

  //There have not been 3 usuccessful attempts to start the engine
  if (starterAttempt < 3)
  {
    if (currentMillis - prevStartAttempt >= engineStartInterval)
    {
      //Serial.println("Attempting to start engine");
      Run_Starter();
      starterAttempt++;
      prevStartAttempt = currentMillis;
    }
  } else
  {
    starterSequence = false;
    starterAttempt = 500;
  }
}

void Run_Starter()
{
  //Run the starter for the specified time
  if (filteredValues.filteredRPM == 0 && filteredValues.filteredSP < 1)
  {
    digitalWrite(START_PIN, HIGH);
    delay(STARTER_TIME);
  } else
  {
    Serial.println("Engine is running, start sequence will be ended.");
    starterSequence = false;
    starterAttempt = 500;
  }

  //Stop the starter
  digitalWrite(START_PIN, LOW);
}
