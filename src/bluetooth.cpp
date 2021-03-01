#include "bluetooth.h"
#include "SharedResources.h"



void InitiallizeBluetooth()
{
  Serial1.begin(115200);
}

void CheckBluetooth(){
  String inputString = "";

  while(Serial1.available())
  {
    inputString = Serial1.readString();

    Serial.print("Bluetooth Input: ");
    Serial.println(inputString);
  }

  if(inputString == "001") {             //requesting bin name
    Serial1.write("Test Bin 1");
  }if(inputString == "002"){            //requesting record
    PublisBluetoothDataMessage();
  }
}

void PublisBluetoothDataMessage() {
  int fanStatus = 0;
  String cableNum = "C0";

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
  char value[100];
  ts.toCharArray(value, 100);
  Serial1.write("ts,");
  Serial1.print(value);
  Serial.print("TIMESTAMP: ");
  Serial.println(value);

  delay(10);

  //doc["msg"] = 0;

  Serial1.write("ap,");

  binaryFloat floatVal;
  floatVal.floatingPoint = Round(filteredValues.filteredAirPressure);
  Serial1.write(floatVal.binary, 4);

  //Serial1.write(Round(filteredValues.filteredAirPressure));
  //doc["ap"] = Round(filteredValues.filteredAirPressure);
/*  doc["ah"] = Round(filteredValues.filteredAirHumidity);
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
  doc["timestamp"] = rtc.getEpoch();
  doc["batchStart"] = config.batchStartTime;
  */
/*
  serializeJson(doc, Serial1);
  //SaveDataRecord(doc);

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

    serializeJson(doc, Serial1);
    serializeJsonPretty(doc, Serial);
    */
    Serial.println();
//  }
}
