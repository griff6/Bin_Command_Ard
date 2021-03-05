#include "bluetooth.h"
#include "SharedResources.h"
#include "sdOperations.h"

unsigned long bleHeartbeat = 1000; //1 second
unsigned long lastReceivedHearbeat = 0;
unsigned long lastSentHeartbeat = 0;
unsigned long delayAfterMsg = 30;

void InitiallizeBluetooth()
{
  Serial1.begin(115200);
}

void CheckBluetooth(){
  String inputString = "";
  String id = "";
  char in[100];

  //Send a connection heartbeat
  if(lastSentHeartbeat < millis())
  {
    Serial1.write("H");
    delay(10);

    lastSentHeartbeat = millis() + bleHeartbeat;
  }

  //Send engine Change State if required
  if(updateBLEengineState)
  {
    delay(10);
    updateBLEengineState = false;
    String val = "fs,";
    val += String(engineState);
    Serial.print("Sending engine State: ");
    Serial.println(val);
    SendMessage(val);
  }

  int availableBytes = Serial1.available();
  for(int i=0; i<availableBytes && i < 100; i++){
    in[i] = Serial1.read();
  }

  inputString = in;

  if(availableBytes > 0 && inputString != "H,"){
    Serial.print("Bluetooth Command Received: ");
    Serial.println(inputString);
  }

while(inputString.indexOf(',') > 0){
  id = inputString.substring(0, inputString.indexOf(','));
  inputString.remove(0, inputString.indexOf(',')+1);


  if(id == "H")
    lastReceivedHearbeat = millis();
  else if(id == "001") {             //requesting bin name
    PublishBluetoothDataMessage();

  }if(id == "002"){            //requesting record
    PublishBluetoothDataMessage();
  }else if(id == "004"){              //turn engine OFF
    userEngineCommand = OFF;
  }else if(id == "005"){            //turn engine ON
    startEngine = true;
    starterAttempt = 0;
    userEngineCommand = ON;
  }else if(id == "006")             //turn engine to AUTO
  {
    //TODO: Handle when the engine gets changed to AUTO
  }
  else if(id == "N"){
    inputString.toCharArray(config.binName, 20);
    SaveConfigFile();
    PublishBluetoothDataMessage();
  }
}

  //Check the received connection heartbeat
  if(lastReceivedHearbeat+5000 > millis())
  {
    bluetoothConnected = true;
  }else{
    //Serial.println("HERE");
    bluetoothConnected = false;
    lastReceivedHearbeat = 0;
  }
}

void PublishBluetoothDataMessage() {
  int fanStatus = 0;
  String cableNum = "C0";
  String val;


  if(filteredValues.filteredRPM > 100)
  {
    fanStatus = 1;
  }

  Serial.println();
  Serial.println("Publishing message for Bluetooth");


  String ts = "ts,";
  ts = ts + print2digits(rtc.getYear());
  ts += "-";
  ts += print2digits(rtc.getMonth());
  ts += "-";
  ts += print2digits(rtc.getDay());
  ts += " ";
  ts += print2digits(rtc.getHours());
  ts += ":";
  ts += print2digits(rtc.getMinutes());


  delay(100);

  SendMessage(ts);

  val = "N,";
  val += String(config.binName);
  Serial.print("Sending bin name: ");
  Serial.println(val);
  SendMessage(val);

  val = "ap,";
  val += String(Round(filteredValues.filteredAirPressure));
  SendMessage(val);

  val = "ah,";
  val += String(Round(filteredValues.filteredAirHumidity));
  SendMessage(val);

  val = "at,";
  val += String(Round(filteredValues.filteredAirTemp));
  SendMessage(val);

  val = "fs,";
  val += String(fanStatus);
  SendMessage(val);

  val = "fh,";
  val += String(Round(filteredValues.filteredFanHumidity));
  SendMessage(val);

  val = "ft,";
  val += String(Round(filteredValues.filteredFanTemp));
  SendMessage(val);

  val = "sp,";
  val += String(Round(filteredValues.filteredSP));
  SendMessage(val);

  val = "minT,";
  val += String(Round(minGrainTemp));
  SendMessage(val);

  val = "maxT,";
  val += String(Round(maxGrainTemp));
  SendMessage(val);

  val = "avT,";
  val += String(Round(avgGrainTemp));
  SendMessage(val);


  val = "minM,";
  val += String(Round(minGrainMoisture));
  SendMessage(val);

  val = "maxM,";
  val += String(Round(maxGrainMoisture));
  SendMessage(val);


  val = "avM,";
  val += String(Round(avgGrainMoisture));
  SendMessage(val);

  val = "mode,";
  val += String(fanMode);
  SendMessage(val);


  val = "bn,";
  val += String(config.batchNumber);
  SendMessage(val);

  val = "ber,";
  val += String(Round(config.batchEngineTime));
  SendMessage(val);

  val = "bat,";
  val += String(Round(config.batchAerateTime));
  SendMessage(val);

  val = "bdt,";
  val += String(Round(config.batchDryingTime));
  SendMessage(val);

  val = "gn,";
  val += String(config.grain);
  SendMessage(val);

  val = "time,";
  val += String(rtc.getEpoch());
  SendMessage(val);

  val = "bStart,";
  val += String(config.batchStartTime);
  SendMessage(val);

  String cbl;
  for(int cable = 0; cable < numCables; cable++)
  {
    cbl = "C";
    cbl += String(cable+1);    //cable identifier
    //val += ",T";              //indicate this is the temperatures

    for(int i = 0; i < cableValues[cable].numSensors; i++)
    {
      bool addedSecond = false;

      val = cbl;
      val += ",";
      val += "T";
      val += String(i);
      val += ",";
      val += String(Round(cableValues[cable].sensors[i].temperature));


      if(i+1 < cableValues[cable].numSensors)
      {
        val += ",";
        val += "T";
        val += String(i+1);
        val += ",";
        val += String(Round(cableValues[cable].sensors[i+1].temperature));
        addedSecond = true;
      }

      SendMessage(val);

      val = cbl;
      val += ",";
      val += "M";
      val += String(i);
      val += ",";
      val += String(Round(cableValues[cable].sensors[i].temperature));

      if(i+1 < cableValues[cable].numSensors)
      {
        val += ",";
        val += "M";
        val += String(i+1);
        val += ",";
        val += String(Round(cableValues[cable].sensors[i+1].moisture));
        addedSecond = true;
      }
      SendMessage(val);

      if(addedSecond)
        i++;
    }

    Serial.println();
  }
}

void SendMessage(String msg)
{
  char value[25];
  msg.toCharArray(value, 100);
  Serial1.write(value);
  Serial.println(value);
  delay(delayAfterMsg);
}
