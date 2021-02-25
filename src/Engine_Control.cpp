#include "Engine_Control.h"
#include "SharedResources.h"


//#define START_PIN 3     //Pin for the start output relay
//#define ACC_PIN 2       //Pin for the Accessories relay

const int STARTER_TIME = 1250;      //time to run the starter in ms
unsigned long engineStartInterval = 10000;  //delay between attempts to start engine
unsigned long prevStartAttempt = 0;
unsigned long warmUpInterval = 60000;
unsigned long coolDownInterval = 300000;
unsigned long stopWarmUpTime = 0;
unsigned long stopCooldDownTime = 0;

bool accPinHigh = false;
//int starterSequence = 0;        //Used to store if the engine should be starting or not
//int starterAttempt = 500;

void EngineController()
{
  //Serial.print("userEngineCommand: ");
  //Serial.print(userEngineCommand);
  //Serial.print("   engineState: ");
  //Serial.println(engineState);

  //If the engine was unable to start don't try again
  if(engineState == FAILED_START){
    return;
  }

//  if(filteredValues.filteredRPM > 100 && engineState != RUNNING)
//  {
//    engineState = RUNNING;
//    updateEngineState = true;
//    startEngine = false;
//  }//else if(filteredValues.filteredRPM < 100 && engineState != STOPPED && startEngine == false)
  //{
  //  engineState = STOPPED;
  //  updateEngineState = true;
  //}



  if(userEngineCommand == OFF && (engineState != COOLDOWN || engineState != STOPPED))
  {
    //Serial.print("userEngineCommand: ");
    //Serial.print(userEngineCommand);
    //Serial.print("  engineState: ");
    //Serial.println(engineState);

    SetThrottle(COOLDOWN);
  }else if(userEngineCommand == ON && (engineState != WARMUP || engineState != RUNNING))
  {
    EngineStartSequence();
  }else if(userEngineCommand == AUTO)
  {

  }

  if(engineState == WARMUP && stopWarmUpTime > millis())
  {
    SetThrottle(DRY);
    stopWarmUpTime = 0;
  }else if(engineState == COOLDOWN && stopCooldDownTime < millis())
  {
    TurnEngineOff();
  }
}

void TurnEngineOff()
{
  Serial.println("Turning Engine OFF");
  digitalWrite(START_PIN, LOW);
  digitalWrite(ACC_PIN, LOW);
  accPinHigh = false;
  startEngine = false;
  starterAttempt = 0;

  Serial.print("engineState: ");
  Serial.println(engineState);

  if(engineState != STOPPED)
    updateEngineState = true;

  engineState = STOPPED;
  stopCooldDownTime = 0;
}

void EngineStartSequence()
{
  if (!startEngine || engineState == WARMUP || engineState == RUNNING)
  {
    return;
  }


  //TODO: Set engine throttle position based off the fanMode (dry/aerate)

  unsigned long currentMillis = millis();

  //There have not been 3 usuccessful attempts to start the engine
  if (starterAttempt < 3)
  {
    digitalWrite(ACC_PIN, HIGH);
    accPinHigh = true;
    if (currentMillis - prevStartAttempt >= engineStartInterval)
    {
      if(starterAttempt == 0)
      {
        SetThrottle(WARMUP);
      }
      //Serial.println("Attempting to start engine");
      Run_Starter();
      starterAttempt++;
      prevStartAttempt = currentMillis;
    }
  } else
  {
    //Serial.println("Failed to start engine");
    digitalWrite(ACC_PIN, LOW);
    accPinHigh = false;
    startEngine = false;
    starterAttempt = 500;
    engineState = FAILED_START;
    updateEngineState = true;
    stopWarmUpTime = 0;
  }
}

void Run_Starter()
{
  //Run the starter for the specified time
  if (filteredValues.filteredRPM == 0)
  {
    //make sure accessories are HIGH
    engineState = STARTING;
    digitalWrite(ACC_PIN, HIGH);
    accPinHigh = true;
    delay(3000);                      //hold accessories on for a period of time to let fuel flow
    Serial.println("running Starter");
    digitalWrite(START_PIN, HIGH);
    delay(STARTER_TIME);
    //Stop the starter
    digitalWrite(START_PIN, LOW);
  } else
  {
    Serial.println("Engine is running, start sequence will be ended.");
    SetThrottle(WARMUP);
    startEngine = false;
    starterAttempt = 500;
    updateEngineState = true;
  }
}

void SetThrottle(int mode)
{
  if(mode == AERATE)
  {
    //TODO: Slow the throttle down to idle
    fanMode = AERATE;
    updateEngineState = true;
  }else if(mode == DRY)
  {
    //TODO: Move to full throttle position
    fanMode = DRY;
    updateEngineState = true;
  }else if(mode == WARMUP)
  {
    //TODO: Move throttle to idle to warm up position

    if(fanMode == AERATE)
    {
      engineState = RUNNING;
    }else if(fanMode == DRY){
      stopWarmUpTime = millis() + warmUpInterval;
      engineState = WARMUP;
      updateEngineState = true;
    }
  }else if(mode == COOLDOWN)
  {
    //TODO: Move throttle to cool down position
    if(fanMode == AERATE){
      Serial.println("In Aerate so turn fan off");
      TurnEngineOff();
      return;
    }else if(engineState != COOLDOWN && engineState != STOPPED){
      Serial.println("Switching to COOLDOWN");
      //Serial.print("userEngineCommand: ");
      //Serial.print(userEngineCommand);
      //Serial.print("  engineState: ");
      //Serial.println(engineState);

      stopCooldDownTime = millis() + coolDownInterval;
      engineState = COOLDOWN;
      updateEngineState = true;
    }
  }
}

void CheckManualStart()
{
  double pinValue = 0;

  //if the controller is in an error state, do nothing
  if(engineState == FAILED_START)
    return;

  digitalWrite(ACC_PIN, LOW);       //Set pin low
  pinMode(ACC_PIN, INPUT_PULLUP);   //switch pin to input and enable internal pullup
  pinValue = analogRead(ACC_PIN);   //sample the pin
  pinMode(ACC_PIN, OUTPUT);         //Set pin back to an output

  //put pin back into the mode it was in before the read
  if(accPinHigh)
    digitalWrite(ACC_PIN, HIGH);
  else
    digitalWrite(ACC_PIN, LOW);

  if(pinValue >= 900)   //pin high
  {
      if(engineState == RUNNING)
      {
        startEngine = false;
        SetThrottle(COOLDOWN);
      }else if(engineState == COOLDOWN || engineState == WARMUP)
      {
          startEngine = false;
          TurnEngineOff();
      }else if(engineState == STOPPED)
      {
          startEngine = true;
          starterAttempt = 0;
          userEngineCommand = ON;
      }
  }
}
