/*  Programmer: Bill Tsou
 *  Date: 2020/02/11  */
#ifndef AD5422_H
#define AD5422_H

#include <SPI.h>

// the address function of register
#define ADDRESS_NOP       0x00
#define ADDRESS_DATA      0x01
#define ADDRESS_READ      0x02
#define ADDRESS_CONTROL   0x55
#define ADDRESS_RESET     0x56

// the read address data
#define READ_STATUS       0x00
#define READ_DATA         0x01
#define READ_CONTROL      0x02

enum OutputMode {
  OUTPUT_MODE_VOLTAGE_0_5 = 0,
  OUTPUT_MODE_VOLTAGE_0_10 = 1,
  OUTPUT_MODE_VOLTAGE_5_5 = 2,
  OUTPUT_MODE_VOLTAGE_10_10 = 3,
  OUTPUT_MODE_CURRENT_4_20 = 5,
  OUTPUT_MODE_CURRENT_0_20 = 6,
  OUTPUT_MODE_CURRENT_0_24 = 7,
  MAX_OUTPUT_MODE_OPTIONS
};

struct StatusRegister
{
  bool CurrentLoopFault;
  bool SlewActive;
  bool OverTemperature;
};

struct ControlRegister
{
  bool ClearSelect;
  bool OverRange;
  bool ExternalResistor;
  bool OutputEnable;
  byte SlewRateClock;
  byte SlewRateStep;
  bool SlewRateEnable;
  bool DaisyChainEnable;
  byte OutputRangeSelect;
};

class AD5422
{
  public:
    inline AD5422() { InitializeAD5422(2, 3); }
    inline AD5422(byte param1, byte param2)
      { InitializeAD5422(param1, param2); }

    bool SetOutputValue(byte, uint16_t = 0);
    // read register data specified above and return
    uint16_t ReadFromRegister(byte);
    
    // set and get the output mode from the enumeration above
    bool SetOutputMode(byte);
    byte GetOutputMode();

    // reset AD5422 using reset pin
    void ResetAD5422();

//    unsigned short TriangularOutput(float);

    // get the status from register, you should call GetAD5422Status() 'before' inspecting the status structure
    void GetAD5422Status();
    // get the control register, you should call GetAD5422Control() 'before' inspecting the status structure
    void GetAD5422ControlRegister();
    // examine whether the fault detection pin has been pull low, which means the current loop is broken
    bool GetCurrentLoopStatus();
    
    // calculate the read output electrical value (voltage or current) and return
    double GetRealOutputValue();
    // return the string pointer that contains the unit according to the output mode right now
    char* GetOutputUnit();

    // 2 reserved register structures according to the fields specified in the datasheet
    ControlRegister controlRegister;
    StatusRegister statusRegister;
  
  private:
    void InitializeAD5422(byte, byte);
    // transform the structure data type to the 16-bit data in order to send to the AD5422 register
    uint16_t ControlRegisterToData();

    // variables for the PIN configurations
    byte PIN_FAULT_DETECTION;   // active LOW   -> current loop broken when pulling LOW
    byte PIN_CLEAR;             // active HIGH  -> pull HIGH can make AD5422 reset

    // reserved data storing the maximum, minimum and digital value of current choosing mode
    // so that GetRealOutputValue() can calculate the value correctly
    short MaxOutputValue;
    short MinOutputValue;
    uint16_t OutputValue;
};

#endif // AD420_H
