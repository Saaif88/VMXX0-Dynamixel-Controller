
#include "stdint.h"                       // allows you to use int8_t, int32_t and others like that 
#include "Arduino.h"                      // Required to use Arduino lingo in VSCODE
#include "Adafruit_FRAM_SPI.h"            // Adafruit FRAM library, contains all of the functions for the FRAM. Needs to be modified to work with the MRAM. Currently using 2.4.0
#include "SPI.h"                          // SPI library, necessary to communicate with the MRAM
#include "wire.h"                         // Required for the FRAM library
#include "Dynamixel2Arduino.h"            // Official Dynamixel control library for Arduino. Needs protocol.c replaced. Currently using 0.6.3

using namespace ControlTableItem;         //This namespace is required to use Control table item names. Required for the Dynamixel Library.

// ********************************************************************************************************************************************************************************************
// Global Variables
// ********************************************************************************************************************************************************************************************

uint8_t DXL_ID = 1;                   // The ID of the Dynamixel
extern bool Fault;                    // A conditional fault variable, used to store the status of a fault condition
long Requested_Position;              // The requested position of the Dynaixel, to be sent to the Goal Position
//extern byte Error;                // Contains the Error Bits. There are 8 possible error bits, so 1 byte is enough to store it.

// ********************************************************************************************************************************************************************************************
// Pin Assignments
// ********************************************************************************************************************************************************************************************

const uint8_t DXL_DIR_PIN = 2; // RS-485 Transciever DIR PIN
uint8_t       FRAM_CS = SS;    // This is the chip select pin on the MRAM, in this case, it is the hardware SS Pin of the Teensy

// ********************************************************************************************************************************************************************************************
// Communications Setup
// ********************************************************************************************************************************************************************************************

//#define DXL_SERIAL Serial1  // Serial Port used to communicate with Dynamixel
//#define PC_SERIAL Serial    // Serial Port used to communicate with the PC
// These lines above have been moved to platformio.ini. They're here so I don't forget where they are.
Dynamixel2Arduino dxl(DXL_SERIAL, DXL_DIR_PIN); // Setup the Dynamixel Serial Port
Adafruit_FRAM_SPI fram = Adafruit_FRAM_SPI(FRAM_CS); // Sets up the MRAM communications, uses hardware SPI