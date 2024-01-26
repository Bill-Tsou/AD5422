/*  Author: Bill Tsou, feel free to use and share the program
 *  Date: 2020/02/11
 *  Function: Manipulate AD5422 in industrial control,
 *    providing 16-bit 4-20mA, 0-20mA, 0-24mA and 0-5V, 0-10V, -5V-5V, -10-10V high-resolution, monotonic output signal
 *  
 *  Remember to look up AD5422 header file for the more information about register-related structure
 *  
 *  Most importantly, you should lift the OUTPUT ENABLE bit in control register to turn on output
 *    REFERENCE UART COMMAND below: *OUTPUT ON / *OUTPUT OFF
*/

#include "AD5422.h"

#define UART_TERMINATOR '\r'
#define MAX_UART_CHARS  50

bool UART_Processing(char *returnString)
{
  char c;
  byte bufferLength = 0;
  do
  {
    c = Serial.read();
    if(c > 0)
    { // the character read from serial buffer is available
      returnString[bufferLength++] = c;
      if(bufferLength >= MAX_UART_CHARS)
        return false;
    }
  }while(c != UART_TERMINATOR);
  returnString[bufferLength - 1] = '\0';
  return true;
}

AD5422 *currentTransmitter;

void setup() {
  Serial.begin(115200);
//  Serial.println("Ready");

  currentTransmitter = new AD5422(6, 7);    // use the default pin configurations

  // remember to enable output after set the digital output value

/*  currentTransmitter->controlRegister.OutputEnable = true;
  currentTransmitter->controlRegister.SlewRateClock = 8;
  currentTransmitter->controlRegister.SlewRateStep = 7;
  currentTransmitter->controlRegister.SlewRateEnable = true;
  currentTransmitter->SetOutputValue(ADDRESS_CONTROL);

  currentTransmitter->SetOutputValue(ADDRESS_DATA, 65535);
  Serial.println(currentTransmitter->ReadFromRegister(READ_DATA));*/
}

void loop() {
  // put your main code here, to run repeatedly:
  
  if(Serial.available())
  {
    char returnString[MAX_UART_CHARS];
    if(UART_Processing(returnString) == false)
      Serial.println("Enter length exceeded!");
    
    else if(strcmp(returnString, "*IDN?") == 0)
      Serial.println("AD5422 16-bit current loop control demo programme, programme designer: Bill Tsou");
    else if(strcmp(returnString, "*RST") == 0)
    {
      currentTransmitter->ResetAD5422();
      currentTransmitter->SetOutputValue(ADDRESS_RESET);
      Serial.println("AD5422 has been reset!");
    }
    
    else if(strcmp(returnString, "*STATUS") == 0)
    { // get the status first
      currentTransmitter->GetAD5422Status();
      Serial.println();
      Serial.println("Status of AD5422: ");
      Serial.print("  Current loop Status: ");
      Serial.println(currentTransmitter->statusRegister.CurrentLoopFault);
      Serial.print("  Slew Active: ");
      Serial.println(currentTransmitter->statusRegister.SlewActive);
      Serial.print("  Over Temperature: ");
      Serial.println(currentTransmitter->statusRegister.OverTemperature);
      Serial.println();
    }

    else if(strcmp(returnString, "*CONTROL") == 0)
    {
      currentTransmitter->GetAD5422ControlRegister();
      Serial.println();
      Serial.println("Control Register of AD5422");
      Serial.print("  Clear Select: ");
      Serial.println(currentTransmitter->controlRegister.ClearSelect);
      Serial.print("  Over Range: ");
      Serial.println(currentTransmitter->controlRegister.OverRange);
      Serial.print("  External Current Setting Resistor: ");
      Serial.println(currentTransmitter->controlRegister.ExternalResistor);
      Serial.print("  Output Enable: ");
      Serial.println(currentTransmitter->controlRegister.OutputEnable);
      Serial.print("  Slew Rate Clock: ");
      Serial.println(currentTransmitter->controlRegister.SlewRateClock);
      Serial.print("  Slew Rate Step: ");
      Serial.println(currentTransmitter->controlRegister.SlewRateStep);
      Serial.print("  Slew Rate Enable: ");
      Serial.println(currentTransmitter->controlRegister.SlewRateEnable);
      Serial.print("  Daisy Chain Enable: ");
      Serial.println(currentTransmitter->controlRegister.DaisyChainEnable);
      Serial.print("  Output Mode Select: ");
      Serial.println(currentTransmitter->controlRegister.OutputRangeSelect);
      Serial.println();
    }

    else if(strcmp(returnString, "*DATA") == 0)
    {
      Serial.print("Digital data in the data register: ");
      Serial.println(currentTransmitter->ReadFromRegister(READ_DATA));
    }

    else if(strcmp(returnString, "*OUTPUT ON") == 0)
    {
      currentTransmitter->controlRegister.OutputEnable = true;
      currentTransmitter->SetOutputValue(ADDRESS_CONTROL);
      Serial.println("Output has been turned on");
    }
    else if(strcmp(returnString, "*OUTPUT OFF") == 0)
    {
      currentTransmitter->controlRegister.OutputEnable = false;
      currentTransmitter->SetOutputValue(ADDRESS_CONTROL);
      Serial.println("Output has been turned off");
    }

    else if(strcmp(returnString, "*MODE") == 0)
    {
      Serial.println("Choose mode from the list: ");
      Serial.println("  1. 0~5V Voltage Output");
      Serial.println("  2. 0~10V Voltage Output");
      Serial.println("  3. -5~5V Voltage Output");
      Serial.println("  4. -10~10V Voltage Output");
      Serial.println("  5. 4~20mA Current Output");
      Serial.println("  6. 0~20mA Current Output");
      Serial.println("  7. 0~24mA Current Output");
      Serial.print("Enter your choice: ");
      UART_Processing(returnString);
      switch(atoi(returnString))
      {
        case 1:
        case 2:
        case 3:
        case 4:
          currentTransmitter->SetOutputMode(atoi(returnString) - 1);
          Serial.print(atoi(returnString));
          Serial.println(".");
        break;

        case 5:
        case 6:
        case 7:
          currentTransmitter->SetOutputMode(atoi(returnString));
          Serial.print(atoi(returnString));
          Serial.println(".");
        break;
        default:
          Serial.println("Invalid option!");
      }
      Serial.println();
    }

    else
    {
      if(currentTransmitter->SetOutputValue(ADDRESS_DATA, (uint16_t)atoi(returnString)) == false)
        Serial.println("Error Occur when converting!");
      else if(currentTransmitter->controlRegister.OutputEnable == false)
        Serial.println("You haven\'t enabled the output by calling the command *OUTPUT ON");
      else
      {
        Serial.print("The digital value has been set to ");
        Serial.print(currentTransmitter->ReadFromRegister(READ_DATA));
        Serial.print(", corresponding output value is: ");
        Serial.print(currentTransmitter->GetRealOutputValue(), 6);
        Serial.print(currentTransmitter->GetOutputUnit());
        Serial.println();
      }
    }
  }
  
  // detect whether the current loop is broken
  static uint16_t errorAcc = 1;
  if(currentTransmitter->GetCurrentLoopStatus() == LOW)
  { // the error occurs
    static unsigned long lastTime = 0;
    if(millis() - lastTime >= 200)
    {
      Serial.print("Check the current loop! Acc: ");
      Serial.println(errorAcc++);
      lastTime = millis();
    }
  }
  else if(errorAcc > 1)
  { // the error has been recovered
    errorAcc = 1;
    Serial.println("The current loop has been fixed!");
  }
}
