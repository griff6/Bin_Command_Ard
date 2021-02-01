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
  cable1.begin();

  uint8_t devices = findDevices(3);
  Serial.print("Found ");
  Serial.print(devices);
  Serial.println(" on cable 0");

}

void GetBinCableValues()
{
  uint8_t numDevices = 0;
  //filteredValues.filtered
  //byte addr[8];

  for(int cable = 0; cable < 3; cable++)
  {
    if(cable == 0)
    {
      cable1.requestTemperatures(); // Send the command to get temperatures
      numDevices = cable1.getDeviceCount();
      delay(10);
    }

    for(int i=0; i < numDevices; i++)
    {
        Serial.print("T");
        Serial.print(i);
        Serial.print(" is: ");
        int temp = cable1.getTempCByIndex(i);
        Serial.print(temp);
        Serial.print(",   ");
        Serial.print("H");
        Serial.print(i);
        Serial.print(" is: ");

        //Calculation is from SHT1x datasheet
        //getMoisByIndex is from a modified one wire library.  Matt Reimer did the modifications
        //https://github.com/mattdreimer/Arduino-Temperature-Control-Library
        unsigned int SO_RH = cable1.getMoisByIndex(i);
        float RH_lin = c1 + c2 * SO_RH + c3 * (SO_RH * SO_RH);//(see SHT11 datasheet for this under section 4)
        float RH_true = (temp - 25) * (t1 + t2 * SO_RH) + RH_lin;
        double moisture = CalculateMoisture(RH_true, temp);

        Serial.print(RH_true);

        Serial.print(",   ");
        Serial.print("M");
        Serial.print(i);
        Serial.print(" is: ");
        Serial.println(moisture);
    }
    Serial.println();
  }
}

float CalculateMoisture(float rh, int temperature){
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

uint8_t findDevices(int pin)
{

  uint8_t address[16];
  uint8_t count = 0;

  if (oneWire1.search(address))
  {
    Serial.print("\nuint8_t pin");
    Serial.print(pin, DEC);
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
    } while (oneWire1.search(address));

    Serial.println("};");
    Serial.print("// nr devices found: ");
    Serial.println(count);
  }
  return count;
}
