#include "bluetooth.h"
#include "SharedResources.h"

unsigned long bleHeartbeat = 1000; //1 second
unsigned long lastReceivedHearbeat = 0;
unsigned long lastSentHeartbeat = 0;

void InitiallizeBluetooth()
{
  Serial1.begin(115200);
}

void CheckBluetooth(){
  String inputString = "";
  String id = "";

  //Send a connection heartbeat
  if(lastSentHeartbeat < millis())
  {
    Serial1.write("H");

    lastSentHeartbeat = millis() + bleHeartbeat;
  }

  while(Serial1.available())
  {
    inputString = Serial1.readString();
    Serial.print("Bluetooth Command Received: ");
    Serial.println(inputString);
  }

while(inputString.indexOf(',') > 0){
  id = inputString.substring(0, inputString.indexOf(','));
  inputString.remove(0, inputString.indexOf(',')+1);

  //Serial.print("id: ");
  //Serial.println(id);

  if(id == "H")
    lastReceivedHearbeat = millis();
  else if(id == "001") {             //requesting bin name
    //TODO: Change this, it is just for testing.
    Serial1.write("Test Bin 1");
  }if(id == "002"){            //requesting record
    PublisBluetoothDataMessage();
  }
}

  //Check the received connection heartbeat
  if(lastReceivedHearbeat+15000 > millis())
  {
    bluetoothConnected = true;
  }else{
    //Serial.println("HERE");
    bluetoothConnected = false;
    lastReceivedHearbeat = 0;
  }
}

void PublisBluetoothDataMessage() {
  int fanStatus = 0;
  String cableNum = "C0";
  String val;
  char value[100];

  if(filteredValues.filteredRPM > 100)
  {
    fanStatus = 1;
  }

  Serial.println();
  Serial.println("Publishing message for Bluetooth");


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
  //size_t capacity = JSON_OBJECT_SIZE(23);
  //size_t capacity = JSON_OBJECT_SIZE(66);
  //DynamicJsonDocument doc(capacity);

  //doc["ts"] = ts;
  ts.toCharArray(value, 100);
  Serial1.write("ts,");
  Serial1.print(value);
  Serial.print("ts: ");
  Serial.println(value);
  delay(10);

  val = "ap,";
  val += String(Round(filteredValues.filteredAirPressure));
  val += ",ah,";
  val += String(Round(filteredValues.filteredAirHumidity));
  val.toCharArray(value, 100);
  Serial1.write(value);
  Serial.println(value);
  delay(10);

  val = "at,";
  if(filteredValues.filteredAirTemp == 65536 || filteredValues.filteredAirTemp == -65536)
    val += " ";
  else
    val += String(Round(filteredValues.filteredAirTemp));
  val += ",fs,";
  val += String(fanStatus);
  val.toCharArray(value, 100);
  Serial1.write(value);
  Serial.println(value);
  delay(10);

  val = "fh,";
  if(filteredValues.filteredFanHumidity == 65536 || filteredValues.filteredFanHumidity == -65536)
    val += " ";
  else
    val += String(Round(filteredValues.filteredFanHumidity));
  val += ",ft,";
  if(filteredValues.filteredFanTemp == 65536 || filteredValues.filteredFanTemp == -65536)
    val += " ";
  else
    val += String(Round(filteredValues.filteredFanTemp));
  val.toCharArray(value, 100);
  Serial1.write(value);
  Serial.println(value);
  delay(10);

  val = "sp,";
  if(filteredValues.filteredSP == 65536 || filteredValues.filteredSP == -65536)
    val += " ";
  else
    val += String(Round(filteredValues.filteredSP));
  val += ",minT,";
  if(minGrainTemp == 65536 || minGrainTemp == -65536)
    val += " ";
  else
    val += String(Round(minGrainTemp));
  val.toCharArray(value, 100);
  Serial1.write(value);
  Serial.println(value);
  delay(10);

  val = "maxT,";
  if(maxGrainTemp == 65536 || maxGrainTemp == -65536)
    val += " ";
  else
    val += String(Round(maxGrainTemp));
  val += ",avT,";
  if(avgGrainTemp == 65536 || avgGrainTemp == -65536)
    val += " ";
  else
    val += String(Round(avgGrainTemp));
  val.toCharArray(value, 100);
  Serial1.write(value);
  Serial.println(value);
  delay(10);


  val = "minM,";
  if(minGrainMoisture == 65536 || minGrainMoisture == -65536)
    val += " ";
  else
    val += String(Round(minGrainMoisture));
  val += ",maxM,";
  if(maxGrainMoisture == 65536 || maxGrainMoisture == -65536)
    val += " ";
  else
    val += String(Round(maxGrainMoisture));
  val.toCharArray(value, 100);
  Serial1.write(value);
  Serial.println(value);
  delay(10);


  val = "avM,";
  if(avgGrainMoisture == 65536 || avgGrainMoisture == -65536)
    val += " ";
  else
    val += String(Round(avgGrainMoisture));
  val += ",mode,";
  val += String(fanMode);
  val.toCharArray(value, 100);
  Serial1.write(value);
  Serial.println(value);
  delay(10);


  val = "bn,";
  val += String(config.batchNumber);
  val.toCharArray(value, 100);
  Serial1.write(value);
  Serial.println(value);
  delay(10);

  val = "ber,";
  val += String(Round(config.batchEngineTime));
  val.toCharArray(value, 100);
  Serial1.write(value);
  Serial.println(value);
  delay(10);

  val = "bat,";
  val += String(Round(config.batchAerateTime));
  val.toCharArray(value, 100);
  Serial1.write(value);
  Serial.println(value);
  delay(10);

  val = "bdt,";
  val += String(Round(config.batchDryingTime));
  val.toCharArray(value, 100);
  Serial1.write(value);
  Serial.println(value);
  delay(10);

  val = "gn,";
  val += String(config.grain);
  val.toCharArray(value, 100);
  Serial1.write(value);
  Serial.println(value);
  delay(10);

  val = "time,";
  val += String(rtc.getEpoch());
  val.toCharArray(value, 100);
  Serial1.write(value);
  Serial.println(value);
  delay(10);

  val = "bStart,";
  val += String(config.batchStartTime);
  val.toCharArray(value, 100);
  Serial1.write(value);
  Serial.println(value);
  delay(10);

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


      if(i+1 < 10)
      {
        val += ",";
        val += "T";
        val += String(i+1);
        val += ",";
        val += String(Round(cableValues[cable].sensors[i+1].temperature));
        addedSecond = true;
      }

      val.toCharArray(value, 100);
      Serial1.write(value);
      Serial.println(value);
      delay(10);

      val = cbl;
      val += ",";
      val += "M";
      val += String(i);
      val += ",";
      val += String(Round(cableValues[cable].sensors[i].temperature));

      if(i+1 < 10)
      {
        val += ",";
        val += "M";
        val += String(i+1);
        val += ",";
        val += String(Round(cableValues[cable].sensors[i+1].moisture));
        addedSecond = true;
      }

      val.toCharArray(value, 100);
      Serial1.write(value);
      Serial.println(value);
      delay(10);

      if(addedSecond)
        i++;
    }

    Serial.println();
  }
}
