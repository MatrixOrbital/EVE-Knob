#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "Eve2_81x.h"           
#include "process.h"
#include "Arduino_AL.h"

File myFile;
char LogBuf[WorkBuffSz];

void setup()
{
  // Initializations.  Order is important
  GlobalInit();
  FT81x_Init();
  SD_Init();

  //---------------------------------------------- Set PWM frequency for D6, D7 & D8 ---------------------------
  // This affects millis() such that each count of millis() is actually 4mS
  
  // TCCR0B = TCCR0B & B11111000 | B00000011;     // set timer 0 divisor to 64 for PWM frequency of 490.20 Hz - default
  TCCR0B = TCCR0B & B11111000 | B00000100;         // set timer 0 divisor to 256 for PWM frequency of 122.55 Hz
  TCCR0A = _BV(COM0A1) | _BV(COM0B1) | _BV(WGM00); // Phase correct half speed PWM
 
  if (!LoadTouchMatrix())
  {
    // We failed to read calibration matrix values from the SD card.
    MakeScreen_Calibrate();
    SaveTouchMatrix();
  }
  
  Cmd_SetRotate(2);  // Rotate the display
  Load_JPG(RAM_G, 0, "Gauge240.jpg");
  Load_JPG(RAM_G + 0x40000UL, 0, "L238x65.jpg");
  MakeScreen_Gauge(0);
  wr8(REG_PWM_DUTY + RAM_REG, 128);      // set backlight

  analogWrite(Servo_PWM_PIN, 0);             // Set Servo to 0 value to match gauge
  
  MainLoop(); // jump to "main()"
}

// MainLoop is called from setup() and it never leaves (which is better than loop() which is called repeatedly)
// Note that if you use loop() instead, you will need to define variables "static" (since loop() is not actually a loop).
// In this infinite loop we might be responding to many different signals including: Virtual key presses on screens, Real
// key presses of Real Keys, Slider touch, and timeouts.
#define TLIMIT     10
#define AngleScale 1
void MainLoop(void)
{
  uint32_t Time2CheckSensor = 100;
  uint32_t Angle, PrevAngle;
  uint32_t DiffTest, AngleDiff;
  int32_t Turn, ServoVal, Value = 0;

  Angle = PrevAngle = AS5048_GetRaw();  // Initialize the working values

  while(1)
  {
    if (millis() >= Time2CheckSensor)
    {
      // Read the angle
      Angle = AS5048_GetRaw();
//      Log("Angle = %ld\n", Angle);
      
      // Detect the direction and magnitude of the change in angle. 
      // We assume that the knob is not capable of being turned more than half a rotation in the sample time
      // so that testing the difference in angle for the smallest value will tell us which direction the knob
      // is turning.
      if (Angle >= PrevAngle)
      {
        DiffTest = Angle - PrevAngle;
        if (DiffTest < 0x2000)               // Is the difference less than half the total?
        {
          if(DiffTest > TLIMIT)
          {
            AngleDiff = DiffTest;
            Turn = -(AngleDiff >> AngleScale);
          }
          else
            Turn = 0;
        }
        else
        {
          AngleDiff = 0x4000 - DiffTest; 
          Turn = AngleDiff >> AngleScale;
        }
      }
      else
      {
        DiffTest = PrevAngle - Angle;
        if (DiffTest < 0x2000)               // Is the difference less than half the total?
        {
          if(DiffTest > TLIMIT)
          {
            AngleDiff = DiffTest;
            Turn = AngleDiff >> AngleScale;
          }
          else
            Turn = 0;
        }
        else
        {
          AngleDiff = 0x4000 - DiffTest; 
          Turn = -(AngleDiff >> AngleScale);
        }
      }

//      Log("Prev:%ld Test:%ld Diff:%ld Turn:%d\n", PrevAngle, DiffTest1, AngleDiff, Turn);
      PrevAngle = Angle;

      Value += Turn;

      // Deal with pinning the needle
      if (Value < 0)
        Value = 0;
      if (Value > 10000)
        Value = 10000;

      MakeScreen_Gauge(Value);

      // good values which move servo are from 16 to 83 or 67 counts 
      ServoVal = Value/149 + 16;
//    Log("servo %d\n", ServoVal);
      analogWrite(Servo_PWM_PIN, ServoVal);             // Set Servo to match gauge - 10000/149 = 67

      Time2CheckSensor = millis() + CheckSensorInterval;
    }
  }
}

// ************************************************************************************
// Following are wrapper functions for C++ Arduino functions so that they may be      *
// called from outside of C++ files.  These are also your opportunity to use a common *
// name for your hardware functions - no matter the hardware.  In Arduino-world you   *
// interact with hardware using Arduino built-in functions which are all C++ and so   *
// your "abstraction layer" must live in this xxx.ino file where C++ works.           *
//                                                                                    *
// This is also an alternative to ifdef-elif hell.  A different target                *
// processor or compiler will include different files for hardware abstraction, but   *
// the core "library" files remain unaltered - and clean.  Applications built on top  *
// of the libraries need not know which processor or compiler they are running /      *
// compiling on (in general and within reason)                                        *
// ************************************************************************************

void GlobalInit(void)
{
  Wire.begin();                          // Setup I2C bus

  Serial.begin(115200);                  // Setup serial port for debug
  while (!Serial) {;}                    // wait for serial port to connect.
  
  // Matrix Orbital Eve display interface initialization
  pinMode(EvePDN_PIN, OUTPUT);            // Pin setup as output for Eve PDN pin.
  digitalWrite(EvePDN_PIN, LOW);          // Apply a resetish condition on Eve
  pinMode(EveChipSelect_PIN, OUTPUT);     // SPI CS Initialization
  digitalWrite(EveChipSelect_PIN, HIGH);  // Deselect Eve
  pinMode(EveAudioEnable_PIN, OUTPUT);    // Audio Enable PIN
  digitalWrite(EveAudioEnable_PIN, LOW);  // Disable Audio
  pinMode(Servo_PWM_PIN, OUTPUT);         // Setup PWM pin for servo

  SPI.begin();                            // Enable SPI
  Log("Startup\n");
}

// Send a single byte through SPI
void SPI_WriteByte(uint8_t data)
{
  SPI.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));
  digitalWrite(EveChipSelect_PIN, LOW);

  SPI.transfer(data);
      
  digitalWrite(EveChipSelect_PIN, HIGH);
  SPI.endTransaction();
}

// Send a series of bytes (contents of a buffer) through SPI
void SPI_WriteBuffer(uint8_t *Buffer, uint32_t Length)
{
  SPI.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));
  digitalWrite(EveChipSelect_PIN, LOW);

  SPI.transfer(Buffer, Length);
      
  digitalWrite(EveChipSelect_PIN, HIGH);
  SPI.endTransaction();
}

// Send a byte through SPI as part of a larger transmission.  Does not enable/disable SPI CS
void SPI_Write(uint8_t data)
{
//  Log("W-0x%02x\n", data);
  SPI.transfer(data);
}

// Read a series of bytes from SPI and store them in a buffer
void SPI_ReadBuffer(uint8_t *Buffer, uint32_t Length)
{
  uint8_t a = SPI.transfer(0x00); // dummy read

  while (Length--)
  {
    *(Buffer++) = SPI.transfer(0x00);
  }
}

// Enable SPI by activating chip select line
void SPI_Enable(void)
{
  SPI.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));
  digitalWrite(EveChipSelect_PIN, LOW);
}

// Disable SPI by deasserting the chip select line
void SPI_Disable(void)
{
  digitalWrite(EveChipSelect_PIN, HIGH);
  SPI.endTransaction();
}

void Eve_Reset_HW(void)
{
  // Reset Eve
  SetPin(EvePDN_PIN, 0);                    // Set the Eve PDN pin low
  MyDelay(50);                             // delay
  SetPin(EvePDN_PIN, 1);                    // Set the Eve PDN pin high
  MyDelay(100);                            // delay
}

void DebugPrint(char *str)
{
  Serial.print(str);
}

// A millisecond delay wrapper for the Arduino function
void MyDelay(uint32_t DLY)
{
  uint32_t wait;
  wait = millis() + DLY; while(millis() < wait);
}

// An abstracted pin write that may be called from outside this file.
void SetPin(uint8_t pin, bool state)
{
  digitalWrite(pin, state); 
}

// An abstracted pin read that may be called from outside this file.
uint8_t ReadPin(uint8_t pin)
{
  return(digitalRead(pin));
}

void SD_Init(void)
{
//  Log("Initializing SD card...\n");
  if (!SD.begin(SDChipSelect_PIN)) 
  {
    Log("SD initialization failed!\n");
    return;
  }
  Log("SD initialization done\n");
}

// Read the touch digitizer calibration matrix values from the Eve and write them to a file
void SaveTouchMatrix(void)
{
  uint8_t count = 0;
  uint32_t data;
  uint32_t address = REG_TOUCH_TRANSFORM_A + RAM_REG;
  
//  Log("Enter SaveTouchMatrix\n");
  
  // If the file exists already from previous run, then delete it.
  if(SD.exists("tmatrix.txt"))
    SD.remove("tmatrix.txt");
  delay(50);
  
  FileOpen("tmatrix.txt", FILEWRITE);
  if(!myFileIsOpen())
  {
    Log("No create file\n");
    FileClose();
    return false;
  }
  
  do
  {
    data = rd32(address + (count * 4));
    Log("TMw: 0x%08lx\n", data);
    FileWrite(data & 0xff);                // Little endian file storage to match Eve
    FileWrite((data >> 8) & 0xff);
    FileWrite((data >> 16) & 0xff);
    FileWrite((data >> 24) & 0xff);
    count++;
  }while(count < 6);
  FileClose();
  Log("Matrix Saved\n");
}

// Read the touch digitizer calibration matrix values from a file and write them to the Eve.
bool LoadTouchMatrix(void)
{
  uint8_t count = 0;
  uint32_t data;
  uint32_t address = REG_TOUCH_TRANSFORM_A + RAM_REG;
  
  FileOpen("tmatrix.txt", FILEREAD);
  if(!myFileIsOpen())
  {
    Log("tmatrix.txt not open\n");
    FileClose();
    return false;
  }
  
  do
  {
    data = FileReadByte() +  ((uint32_t)FileReadByte() << 8) + ((uint32_t)FileReadByte() << 16) + ((uint32_t)FileReadByte() << 24);
    Log("TMr: 0x%08lx\n", data);
    wr32(address + (count * 4), data);
    count++;
  }while(count < 6);
  
  FileClose();
  Log("Matrix Loaded \n");
  return true;
}

// ************************************************************************************
// Following are abstracted file operations for Arduino.  This is possible by using a * 
// global pointer to a single file.  It is enough for our needs and it hides file     *
// handling details within the abstraction.                                           *
// ************************************************************************************
void FileOpen(char *filename, uint8_t mode)
{
  // Since one also loses access to defined values like FILE_READ from outside the .ino
  // I have been forced to make up values and pass them here (mode) where I can use the 
  // Arduino defines.
  switch(mode)
  {
  case FILEREAD:
    myFile = SD.open(filename, FILE_READ);
    break;
  case FILEWRITE:
    myFile = SD.open(filename, FILE_WRITE);
    break;
  default:;
  }
}

void FileClose(void)
{
  myFile.close();
  if(myFileIsOpen())
  {
    Log("Failed to close file\n");
  }
}

// Read a single byte from a file
uint8_t FileReadByte(void)
{
  return(myFile.read());
}

// Read bytes from a file into a provided buffer
void FileReadBuf(uint8_t *data, uint32_t NumBytes)
{
  myFile.read(data, NumBytes);
}

void FileWrite(uint8_t data)
{
  myFile.write(data);
}

uint32_t FileSize(void)
{
  return(myFile.size());
}

uint32_t FilePosition(void)
{
  return(myFile.position());
}

bool FileSeek(uint32_t offset)
{
  return(myFile.seek(offset));
}

bool myFileIsOpen(void)
{
  if(myFile)
    return true;
  else
    return false;
}

uint16_t AS5048_GetRaw(void) 
{
  uint8_t returned;
  uint16_t Reg_Val = 0;

  Wire.beginTransmission(I2C_Address);
  Wire.write(AS5048_REG_ANG);
  returned = Wire.endTransmission(false);
  if (returned)
    Log("AS5048_Read16 error: %d", returned);

  Wire.requestFrom(I2C_Address, 2);
  while(Wire.available() < 2);        // Wait here until it is ready

  Reg_Val = ( ((uint16_t)Wire.read()) << 6 );
  Reg_Val += Wire.read() & 0x3F;
  
  return (Reg_Val);
}


