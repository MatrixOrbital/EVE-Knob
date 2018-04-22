A small quantity of joy is added to the Universe each time a knob is spun.

Using the AS5048B from AMS allows the knob to rotate freely and infinitely while reading the position via I2C. AMS makes a handy and inexpensive eval-board which is available from Mouser: [985-AS5048A-TS_EK_AB](https://www.mouser.com/ProductDetail/ams/AS5048B-TS_EK_AB?qs=%2fha2pyFadujZZKz1OfAGsHrNpvDGr%252bCw2bjn7ei0D4lkCfqSNlRtkoUgtMJtRFAa)

In essence, this is a rotational encoder capable of 1/20th degree resolution (7300 counts per revolution). The technology is also useful in robotics to quantify wheel rotation or joint extension.

- Designed for Matrix Orbital EVE2 SPI TFT Displays

  https://www.matrixorbital.com/ftdi-eve

- This code makes use of the Matrix Orbital EVE2 Library found here: 

  https://github.com/MatrixOrbital/EVE2-Library

  - While a copy of the library files (Eve2_81x.c and Eve2_81x.h) is included here, you may look for updated
    files if you wish.  This is optional, but the Eve2-Library is likely to contain an extension of what you
    have here in case you wish to make some more advanced screens.

- Matrix Orbital EVE2 SPI TFT display information can be found at: https://www.matrixorbital.com/ftdi-eve

- An Arduino shield with a connector for Matrix Orbital EVE2 displays is used to interface the Arduino to Eve.  
  This shield includes:
  - 20 contact FFC connector for Matrix Orbital EVE2 displays
  - 3 pushbuttons for application control without requiring a touchscreen (useful for initial calibration)
  - Audio amplifier and speaker for audio feedback
  - SD card holder
  - Additionally, the shield board is automatically level shifted for 5V Arduino and works with 3.3V Parallax Propeller ASC+ 
  
  Support Forums
  
  http://www.lcdforums.com/forums/viewforum.php?f=45
