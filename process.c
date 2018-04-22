// Process.c is the application layer.  All function calls are hardware ambivalent.

#include <stdint.h>              // Find integer types like "uint8_t"  
#include "Eve2_81x.h"             // Matrix Orbital Eve2 Driver
#include "Arduino_AL.h"          // include the hardware specific abstraction layer header for the specific hardware in use.
#include "process.h"             // Every c file has it's header and this is the one for this file

void MakeScreen_Gauge(uint16_t Val)
{
  Log("Enter NiceGauge\n");

  Send_CMD(CMD_DLSTART);
  Send_CMD(CLEAR(1,1,1));
//  Cmd_SetRotate(2);

  // Define the bitmap
  Send_CMD(BITMAP_HANDLE(1));                                        // handle for this bitmap
  Cmd_SetBitmap(RAM_G, RGB565, 240, 240);                            // Use the CoPro Command to fill in the bitmap parameters (Eve PG 5.6.5)
  Send_CMD(BEGIN(BITMAPS));
  Send_CMD(VERTEX2II(0, 2, 1, 0));
  
  // Define the other bitmap
  Send_CMD(BITMAP_HANDLE(2));                                        // handle for this bitmap
  Cmd_SetBitmap(RAM_G + 0x40000UL, RGB565, 238, 65);                     // Use the CoPro Command to fill in the bitmap parameters (Eve PG 5.6.5)
  Send_CMD(VERTEX2II(1, 252, 2, 0));
  
  Send_CMD(COLOR_RGB(128, 255, 192));

  // This "gauge" is nothing more than a rotating pin (on top of a nice bitmap)
  Cmd_Gauge(120, 120, 100, OPT_NOBACK | OPT_NOTICKS, 0, 4, Val, 10000);

  Send_CMD(DISPLAY());
  Send_CMD(CMD_SWAP);
  UpdateFIFO();                                               // Trigger the CoProcessor to start processing commands out of the FIFO
}

// A calibration screen for the touch digitizer
void MakeScreen_Calibrate(void)
{
  Log("Enter Calibrate\n");
  
  Send_CMD(CMD_DLSTART);
  Send_CMD(CLEAR_COLOR_RGB(0,0,0));
  Send_CMD(CLEAR(1,1,1));
  Cmd_Text(100, 240, 27, OPT_CENTER, "Tap on the dots");
  Cmd_Calibrate(0);                                           // This widget generates a blocking screen that doesn't unblock until 3 dots have been touched
  Send_CMD(DISPLAY());
  Send_CMD(CMD_SWAP);
  UpdateFIFO();                                               // Trigger the CoProcessor to start processing commands out of the FIFO
  
  Wait4CoProFIFOEmpty();                                      // wait here until the coprocessor has read and executed every pending command.
  MyDelay(100);

  Log("Leaving Calibrate\n");
}

// This define is for the size of the buffer we are going to use for data transfers.  It is 
// sitting here so uncomfortably because it is a silly tiny buffer in Arduino Uno and you
// will want a bigger one if you can get it.  Redefine this and add a nice buffer to Load_ZLIB()
#define COPYBUFSIZE WorkBuffSz

// Load a JPEG image from SD card into RAM_G at address "BaseAdd"
// Return value is the last RAM_G address used during the jpeg decompression operation.
uint32_t Load_JPG(uint32_t BaseAdd, uint32_t Options, char *filename)
{
  uint32_t Remaining;
  uint32_t ReadBlockSize = 0;

  // Open the file on SD card by name
  FileOpen(filename, FILEREAD);
  if(!myFileIsOpen())
  {
    Log("%s not open\n", filename);
    FileClose();
    return false;
  }
  
  Remaining = FileSize();                                      // Store the size of the currently opened file
  
  Send_CMD(CMD_LOADIMAGE);                                     // Tell the CoProcessor to prepare for compressed data
  Send_CMD(BaseAdd);                                           // This is the address where decompressed data will go 
  Send_CMD(Options);                                           // Send options (options are mostly not obviously useful)

  while (Remaining)
  {
    if (Remaining > COPYBUFSIZE)
      ReadBlockSize = COPYBUFSIZE;
    else
      ReadBlockSize = Remaining;
    
    FileReadBuf(LogBuf, ReadBlockSize); // Read a block of data from the file
    
    // write the block to FIFO
    CoProWrCmdBuf(LogBuf, ReadBlockSize);                    // Does FIFO triggering
  
    // Calculate remaining
    Remaining -= ReadBlockSize;                              // Reduce remaining data value by amount just read
    // Log("Remaining = %ld RBS = %ld\n", Remaining, ReadBlockSize);
  }
  FileClose();

  Wait4CoProFIFOEmpty();                                     // wait here until the coprocessor has read and executed every pending command.

  // Get the address of the last RAM location used during inflation
  Cmd_GetPtr();                                              // FifoWriteLocation is updated twice so the data is returned to it's updated location - 4
  UpdateFIFO();                                              // force run the GetPtr command
  return (rd32(FifoWriteLocation + RAM_CMD - 4));            // The result is stored at the FifoWriteLocation - 4
}         
