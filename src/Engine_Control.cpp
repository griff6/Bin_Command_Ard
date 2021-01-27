#include "Engine_Control.h"
#include "SharedResources.h"


#define START_PIN 2     //Pin for the start output relay
#define ACC_PIN 1       //Pin for the Accessories relay

const int STARTER_TIME = 1250;      //time to run the starter in ms
unsigned long engineStartInterval = 10000;  //delay between attempts to start engine
unsigned long prevStartAttempt = 0;
//int starterSequence = 0;        //Used to store if the engine should be starting or not
//int starterAttempt = 500;

void EngineController()
{
  //Serial.print("userEngineCommand: ");
  //Serial.print(userEngineCommand);
  //Serial.print("   engineState: ");
  //Serial.println(engineState);

  if(filteredValues.filteredRPM > 100 && engineState != RUNNING)
  {
    engineState = RUNNING;
    updateEngineState = true;
    startEngine = false;
  }//else if(filteredValues.filteredRPM < 100 && engineState != STOPPED && startEngine == false)
  //{
  //  engineState = STOPPED;
  //  updateEngineState = true;
  //}

  if(userEngineCommand == OFF && engineState != STOPPED)
  {
    TurnEngineOff();
  }else if(userEngineCommand == ON && engineState != RUNNING)
  {
    EngineStartSequence();
  }else if(userEngineCommand == AUTO)
  {

  }
}

void TurnEngineOff()
{
  Serial.println("Turning Engine OFF");
  digitalWrite(START_PIN, LOW);
  digitalWrite(ACC_PIN, LOW);
  startEngine = false;
  starterAttempt = 0;
  engineState = STOPPED;
  updateEngineState = true;
}

void EngineStartSequence()
{
  if (!startEngine)
  {
    return;
  }

  unsigned long currentMillis = millis();

  //There have not been 3 usuccessful attempts to start the engine
  if (starterAttempt < 3)
  {
    digitalWrite(ACC_PIN, HIGH);
    if (currentMillis - prevStartAttempt >= engineStartInterval)
    {
      //Serial.println("Attempting to start engine");
      Run_Starter();
      starterAttempt++;
      prevStartAttempt = currentMillis;
    }
  } else
  {
    startEngine = false;
    starterAttempt = 500;
  }
}

void Run_Starter()
{
  //Run the starter for the specified time
  if (filteredValues.filteredRPM == 0)
  {
    //make sure accessories are HIGH
    digitalWrite(ACC_PIN, HIGH);
    delay(3000);                      //hold accessories on for a period of time to let fuel flow
    Serial.println("running Starter");
    digitalWrite(START_PIN, HIGH);
    delay(STARTER_TIME);
    //Stop the starter
    digitalWrite(START_PIN, LOW);
  } else
  {
    Serial.println("Engine is running, start sequence will be ended.");
    startEngine = false;
    starterAttempt = 500;
    updateEngineState = true;
  }
}
