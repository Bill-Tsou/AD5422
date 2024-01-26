/*  Programmer: Bill Tsou
 *  Date: 2020/02/11  */
#include "AD5422.h"

void AD5422::InitializeAD5422(byte faultDetection, byte clear)
{
  PIN_FAULT_DETECTION = faultDetection;
  PIN_CLEAR = clear;
  
  pinMode(SS, OUTPUT);
  pinMode(PIN_CLEAR, OUTPUT);

  pinMode(PIN_FAULT_DETECTION, INPUT_PULLUP);
  
  digitalWrite(SS, HIGH);
  digitalWrite(PIN_CLEAR, LOW);

  // reset the register contents
  digitalWrite(SS, LOW);
  delay(2);
  digitalWrite(SS, HIGH);

  // initialize the control register
  controlRegister.ClearSelect = false;
  controlRegister.OverRange = false;
  controlRegister.ExternalResistor = false;
  controlRegister.OutputEnable = false;
  controlRegister.SlewRateClock = 0;
  controlRegister.SlewRateStep = 0;
  controlRegister.SlewRateEnable = false;
  controlRegister.DaisyChainEnable = false;
  controlRegister.OutputRangeSelect = 0;

  SPI.begin();
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));

  // reset the chip
  SetOutputValue(ADDRESS_RESET);
  
  // default range using 4-20mA current output
  SetOutputMode(OUTPUT_MODE_CURRENT_4_20);
  SetOutputValue(ADDRESS_CONTROL);
}

bool AD5422::SetOutputValue(byte address, uint16_t data)
{
  uint16_t returnValue;

  if(address == ADDRESS_RESET)
    data = 0x0001;
  else if(address == ADDRESS_CONTROL)
    data = ControlRegisterToData();

  digitalWrite(SS, LOW);
    SPI.transfer(address);
    SPI.transfer16(data);
  digitalWrite(SS, HIGH);

  // get the output value from register
  if(address == ADDRESS_DATA)
  {
    OutputValue = data;
    returnValue = ReadFromRegister(READ_DATA);

    // check whether the value is correct
    if(returnValue != OutputValue)
      return false; // the value has not been set properly
  }
  return true;
}

uint16_t AD5422::ReadFromRegister(byte address)
{
  uint16_t returnValue;

  // set the register desired to read
  SetOutputValue(ADDRESS_READ, address);
  
  delay(2);
  
  digitalWrite(SS, LOW);
    SPI.transfer(ADDRESS_NOP);
    returnValue = SPI.transfer16(0x0000);
  digitalWrite(SS, HIGH);

  return returnValue;
}

bool AD5422::SetOutputMode(byte outputMode)
{
  if((outputMode >= MAX_OUTPUT_MODE_OPTIONS) || (outputMode == 4))
    return false;
  else
    controlRegister.OutputRangeSelect = outputMode;

  switch(controlRegister.OutputRangeSelect)
  {
  case OUTPUT_MODE_VOLTAGE_0_5:
      MaxOutputValue = 5;
      MinOutputValue = 0;
      break;

  case OUTPUT_MODE_VOLTAGE_0_10:
      MaxOutputValue = 10;
      MinOutputValue = 0;
      break;

  case OUTPUT_MODE_VOLTAGE_5_5:
      MaxOutputValue = 5;
      MinOutputValue = -5;
      break;

  case OUTPUT_MODE_VOLTAGE_10_10:
      MaxOutputValue = 10;
      MinOutputValue = -10;
      break;

  case OUTPUT_MODE_CURRENT_4_20:
      MaxOutputValue = 20;
      MinOutputValue = 4;
      break;

  case OUTPUT_MODE_CURRENT_0_20:
      MaxOutputValue = 20;
      MinOutputValue = 0;
      break;

  case OUTPUT_MODE_CURRENT_0_24:
      MaxOutputValue = 24;
      MinOutputValue = 0;
      break;
  }
  SetOutputValue(ADDRESS_CONTROL);
  return true;
}

void AD5422::ResetAD5422()
{
  digitalWrite(PIN_CLEAR, HIGH);
  delay(10);
  digitalWrite(PIN_CLEAR, LOW);
}

uint16_t AD5422::ControlRegisterToData()
{
  return (controlRegister.ClearSelect << 15) |
         (controlRegister.OverRange << 14) |
         (controlRegister.ExternalResistor << 13) |
         (controlRegister.OutputEnable << 12) |
         (controlRegister.SlewRateClock << 8) |
         (controlRegister.SlewRateStep << 5) |
         (controlRegister.SlewRateEnable << 4) |
         (controlRegister.OutputRangeSelect);
}

/*
unsigned short AD5422::TriangularOutput(float period)
{ // keep on tracking that the button is at low (unpressed) status
  double i = 0;
  double steppingValue = 65535.0f / (period / 2.0f * 1000.0f);
  do
  {
    for(; ((int32_t)i <= 65535) && (digitalRead(PIN_TERMINATION_BUTTON) == LOW) && GetAD420Status(); i += steppingValue)
    {
      SetOutputValue((uint16_t)i);
      delay(1);
    }
    if((int32_t)i > 65535)
      i = 65535.0f;
    for(; ((int32_t)i >= 0) && (digitalRead(PIN_TERMINATION_BUTTON) == LOW) && GetAD420Status(); i -= steppingValue)
    {
      SetOutputValue((uint16_t)i);
      delay(1);
    }
    if((int32_t)i < 0)
      i = 0.0f;

    // check whether any error occurs
    if(GetAD5422Status() == LOW)
      return ERROR_CODE;
  }while(digitalRead(PIN_TERMINATION_BUTTON) == LOW);
  return SUCCESS_CODE;
}*/

void AD5422::GetAD5422Status()
{
  uint16_t returnValue = ReadFromRegister(READ_STATUS);
  statusRegister.OverTemperature = returnValue & 0x0001;
  statusRegister.SlewActive = returnValue & (0x0001 << 1);
  statusRegister.CurrentLoopFault = returnValue & (0x0001 << 2);
}

void AD5422::GetAD5422ControlRegister()
{
  uint16_t returnValue = ReadFromRegister(READ_CONTROL);
  controlRegister.ClearSelect = (returnValue >> 15) & 0x0001;
  controlRegister.OverRange = (returnValue >> 14) & 0x0001;
  controlRegister.ExternalResistor = (returnValue >> 13) & 0x0001;
  controlRegister.OutputEnable = (returnValue >> 12) & 0x0001;
  controlRegister.SlewRateClock = (returnValue >> 8) & 0x000F;
  controlRegister.SlewRateStep = (returnValue >> 5) & 0x0007;
  controlRegister.SlewRateEnable = (returnValue >> 4) & 0x0001;
  controlRegister.DaisyChainEnable = (returnValue >> 3) & 0x0001;
  controlRegister.OutputRangeSelect = returnValue & 0x0007;
}

bool AD5422::GetCurrentLoopStatus()
{  return digitalRead(PIN_FAULT_DETECTION);  }

byte AD5422::GetOutputMode()
{  return controlRegister.OutputRangeSelect;  }

char* AD5422::GetOutputUnit()
{  return ((GetOutputMode() <= OUTPUT_MODE_VOLTAGE_10_10) ? "V" : "mA");  }

double AD5422::GetRealOutputValue()
{  return ((double)OutputValue / 65535.0f * (double)(MaxOutputValue - MinOutputValue) + MinOutputValue);  }
