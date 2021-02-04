#include "binCables.h"
#include <OneWire.h>
#include "DallasTemperature.h"
#include "SharedResources.h"
#include <Math.h>

// Setup a oneWire instance to communicate with any OneWire devices
OneWire wires[3] = {OneWire(3), OneWire(4), OneWire(5)};
//OneWire oneWire1(3);  //Cable 1 is connected to pin 3
//OneWire oneWire2(4);  //Cable 2 is connected to pin 4
//OneWire oneWire3(5);  //Cable 3 is connected to pin 5

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature cables[3] = {DallasTemperature(&wires[0]), DallasTemperature(&wires[1]), DallasTemperature(&wires[2])};
//DallasTemperature cable1(&oneWire1);
//DallasTemperature cable2(&oneWire2);
//DallasTemperature cable3(&oneWire3);

//Constants for converting humidity.  Taken from SHT11 datasheet
const double c1 = -2.0468;
const double c2 = 0.0367;
const double c3 = -0.0000015955;
const double t1 = 0.01;
const double t2 = 0.00008;

void SetupBinCables()
{
  // Start up the library
  cables[0].begin();
  cables[1].begin();
  cables[2].begin();

  GetMinMaxAddresses();
/*
  for(int i = 0; i < 3; i++)
  {
    uint8_t devices = findDevices(i);
    Serial.print("Found ");
    Serial.print(devices);
    Serial.print(" on cable ");
    Serial.println(i);
  }
*/
}

void GetBinCableValues()
{
  uint8_t numDevices = 0;
  int addr;
  float temp;
  int index;
  float RH_lin;
  float RH_true;
  double moisture;
  unsigned int SO_RH;

  maxTemp = 0;
  minTemp = 65536;
  avgTemp = 0;
  maxMoisture = 0;
  minMoisture =65536;
  avgMoisture = 0;

  for(int cable = 0; cable < 3; cable++)
  {
    cables[cable].requestTemperatures(); // Send the command to get temperatures
    numDevices = cables[cable].getDeviceCount();
    delay(10);

    cableValues[cable].avgM = 0;
    cableValues[cable].avgT = 0;

    for(int i=0; i < numDevices; i++)
    {
        addr = cables[cable].getUserDataByIndex(i);
        temp = cables[cable].getTempCByIndex(i);
        //Calculation is from SHT1x datasheet
        //getMoisByIndex is from a modified one wire library.  Matt Reimer did the modifications
        //https://github.com/mattdreimer/Arduino-Temperature-Control-Library
        SO_RH = cables[cable].getMoisByIndex(i);
        RH_lin = c1 + c2 * SO_RH + c3 * (SO_RH * SO_RH);//(see SHT11 datasheet for this under section 4)
        RH_true = (temp - 25) * (t1 + t2 * SO_RH) + RH_lin;
        moisture = CalculateMoisture(RH_true, temp);
        index = addr - cableValues[cable].minAddress;

        cableValues[cable].sensors[index].humidity = RH_true;
        cableValues[cable].sensors[index].address = addr;
        cableValues[cable].sensors[index].temperature = temp;
        cableValues[cable].sensors[index].moisture = moisture;

        if(temp > maxTemp)
          maxTemp = temp;
        if(temp < minTemp)
          minTemp = temp;

        if(moisture > maxMoisture)
          maxMoisture = moisture;
        if(moisture < minMoisture)
          minMoisture = moisture;

        cableValues[cable].avgM += moisture;
        cableValues[cable].avgT += temp;
        //Serial.print("avgT: ");
        //Serial.println(cableValues[cable].avgT);
/*
        Serial.print("Address: ");
        Serial.print(cableValues[cable].sensors[index].address);
        Serial.print(",   ");
        Serial.print("T");
        Serial.print(i);
        Serial.print(" is: ");
        Serial.print(cableValues[cable].sensors[index].temperature);
        Serial.print(",   ");
        Serial.print("H");
        Serial.print(i);
        Serial.print(" is: ");
        Serial.print(cableValues[cable].sensors[index].humidity);
        Serial.print(",   ");
        Serial.print("M");
        Serial.print(i);
        Serial.print(" is: ");
        Serial.println(cableValues[cable].sensors[index].moisture);
        */

    }
    cableValues[cable].avgM = cableValues[cable].avgM / (numDevices);
    cableValues[cable].avgT = cableValues[cable].avgT / (numDevices);

    if(numDevices > 0)
    {
      avgMoisture += cableValues[cable].avgM;
      avgTemp += cableValues[cable].avgT;
    }

    Serial.print("Cable ");
    Serial.print(cable);
    Serial.print(" Average Temperature is ");
    Serial.print(cableValues[cable].avgT);
    Serial.print(" Average Moisture is ");
    Serial.print(cableValues[cable].avgM);
    Serial.println();

  }

  if(numCables > 0){
    avgMoisture = avgMoisture / numCables;
    avgTemp = avgTemp / numCables;
  }
/*
  Serial.print(" Average Temperature is ");
  Serial.print(avgTemp);
  Serial.print(" Average Moisture is ");
  Serial.print(avgMoisture);
  Serial.print(" Max Temperature is ");
  Serial.print(maxTemp);
  Serial.print(" Min Temperature is ");
  Serial.print(minTemp);
  Serial.print(" Max Moisture is ");
  Serial.print(maxMoisture);
  Serial.print(" Min Moisture is ");
  Serial.print(minMoisture);
  Serial.println();
  */

}

void GetMinMaxAddresses()
{
  uint8_t numDevices = 0;

  for(int cable = 0; cable < 3; cable++)
  {
//    cables[cable].requestTemperatures(); // Send the command to get temperatures
    numDevices = cables[cable].getDeviceCount();
    delay(10);

    if(numDevices > 0)
      numCables++;

    cableValues[cable].numSensors = numDevices;

    for(int i=0; i < numDevices; i++)
    {
        int addr = cables[cable].getUserDataByIndex(i);

        if(addr < cableValues[cable].minAddress)
          cableValues[cable].minAddress = addr;

        if(addr > cableValues[cable].maxAddress)
          cableValues[cable].maxAddress = addr;
    }

/*
    Serial.print("Cable: ");
    Serial.print(cable);
    Serial.print("  Min Address: ");
    Serial.print(cableValues[cable].minAddress);
    Serial.print("  Max Address: ");
    Serial.println(cableValues[cable].maxAddress);
    */
  }
}

double CalculateMoisture(float rh, int temperature){
  double A = 0;
  double B = 0;
  double C = 0;
  double MCdb = 0;
  double mc = 0;
  String eqn = "";

  if(strcmp(config.grain, "Canola") == 0)
  {
    //From ASAE D245.5
    A = 0.000103;
    B = 1.6129;
    C = 89.99;
    eqn = "Henderson";
  }else if(strcmp(config.grain, "Wheat") == 0)
  {
    //From ASAE D245.5
    A = 0.000043295;
    B = 2.1119;
    C = 41.565;
    eqn = "Henderson";
  }else if(strcmp(config.grain, "Barley") == 0)
  {
    //From ASAE D245.5
    A = 475.12;
    B = 0.14843;
    C = 71.996;
    eqn = "Chung-Pfost";
  }

  if(eqn == "Henderson"){
    MCdb = (log(1-rh / 100)) / (-A * (temperature + C));
    MCdb = pow(MCdb, (1/B));
  }else if(eqn == "Chung-Pfost"){
    MCdb = (-1 / B) * log((-1 * (temperature + C) * log(rh / 100)) / A);   //Gets the dry basis moisture content based off Chung-Pfost equation
  }

  mc = (100 * MCdb) / (100 + MCdb);

  return mc;
}

uint8_t findDevices(int cable)
{

  uint8_t address[16];
  uint8_t count = 0;

  if (wires[cable].search(address))
  {
    Serial.print("\nuint8_t pin");
    Serial.print(cable, DEC);
    Serial.println("[][8] = {");
    do {
      count++;
      Serial.println("  {");
      for (uint8_t i = 0; i < 8; i++)
      {
        Serial.print("0x");
        if (address[i] < 0x10) Serial.print("0");
          Serial.print(address[i], HEX);
        if (i < 7)
          Serial.print(", ");
      }
      Serial.println("  },");
    } while (wires[cable].search(address));

    Serial.println("};");
    Serial.print("// nr devices found: ");
    Serial.println(count);
  }
  return count;
}
