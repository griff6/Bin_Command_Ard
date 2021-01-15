#include "BME280.h"
#include "SharedResources.h"

#define I2C_ADDRESS 0x76  //address for BME280 sensor

//create a BMx280I2C object using the I2C interface with I2C Address 0x76
BMx280I2C bmx280(I2C_ADDRESS);

float bmp_fc = .75;//.2;            // higher value puts less filtering

void SetupBME280()
{
  //begin() checks the Interface, reads the sensor ID (to differentiate between BMP280 and BME280)
  //and reads compensation parameters.
  if (!bmx280.begin())
  {
    Serial.println("begin() failed. check your BMx280 Interface and I2C Address.");
    //while (1);
  }

//  if (bmx280.isBME280())
//    Serial.println("sensor is a BME280");
//  else
//    Serial.println("sensor is a BMP280");

  //reset sensor to default parameters.
  bmx280.resetToDefaults();

  //by default sensing is disabled and must be enabled by setting a non-zero
  //oversampling setting.
  //set an oversampling setting for pressure and temperature measurements.
  bmx280.writeOversamplingPressure(BMx280MI::OSRS_P_x16);
  bmx280.writeOversamplingTemperature(BMx280MI::OSRS_T_x16);

  //if sensor is a BME280, set an oversampling setting for humidity measurements.
  if (bmx280.isBME280())
    bmx280.writeOversamplingHumidity(BMx280MI::OSRS_H_x16);
}

void PrintBME280Data()
{
  float temp (NAN), pres (NAN), hum (NAN);

  //start a measurement
  if (!bmx280.measure())
  {
    Serial.println("could not start measurement, is a measurement already running?");
    return;
  }

  //wait for the measurement to finish
  do
  {
    delay(100);
  } while (!bmx280.hasValue());

  pres = bmx280.getPressure64();
  temp = bmx280.getTemperature();

  //important: measurement data is read from the sensor in function hasValue() only.
  //make sure to call get*() functions only after hasValue() has returned true.
  if (bmx280.isBME280())
  {
    hum = bmx280.getHumidity();
  }

  filterValue(pres, filteredValues.filteredAirPressure, bmp_fc);
  filterValue(temp, filteredValues.filteredAirTemp, bmp_fc);
  filterValue(hum, filteredValues.filteredAirHumidity, bmp_fc);

  Serial.print("Ambient Temperature: ");
  Serial.println(filteredValues.filteredAirTemp);
  //Serial.println(filteredAmbientT);
  Serial.print("Ambient Humidity: ");
 // Serial.println(filteredAmbientH);
 Serial.println(filteredValues.filteredAirHumidity);
  Serial.print("Air Pressure: ");
  //Serial.println(filteredAirPressure);
  Serial.println(filteredValues.filteredAirPressure);
}
