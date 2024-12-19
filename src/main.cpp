// This is designed to be used with a Arduino Micro or a TEENSY 3.2/LC
// This is also designed to be used with an Everspin MR25H256 MRAM Module
// CHANGELOG: Modified Dynamixel library with new protocol.c file to fix -131073 Bug** Not sure this change is still relevant
// CHANGELOG: Using new Dynamixel Library from Jonathon which supports Dynamixel Y
// CHANGELOG: Added new #IFDEF Statements to switch easily between DXL Y, DXL Pro and MX Series
// CHANGELOG: Adding Support for Dynamixel Y
// Fixes: Added ping checks in all main functions
// Version Info : 2.6
// TODO: Replace Teensy 3.2 with Teensy 4.0

/*
   What is this code? This program is used to communicate to a Dynamixel Servo Motor. This servo motor has it's own proprietary communication protocol, so in order for the Arduino to communicate with it, the Dynamixel2Arduino library is used.
   The ultimate goal of this code is to allow a PC to communicate with a Dynamixel via serial. This could be done simply with a logic translator, but the Arduino is used to serve multiple important purposes.
   The main problem with Dynamixels is that they do not have any real non-volatile memory. Whenever a Dynamixel is power cycled, the values stored in memory are reset to the default setting. This means that the PID values, speed, torque and more need to be initialized.
   The Arduino is used to initalize these parameters whenever the system is turned on. The arduino is also connected to something called MRAM. This is a special kind of non-volatile memory that has infinite read/write cycles.
   This is used to keep track of the position of the Dynamixel over more than one turn. The Dynamixel has no real non-volatile memory, but it does have an absolute encoder. This encoder keeps track of the servo between 0-4096 counts. Any more or less is lost upon reboot.
   The MRAM holds the positional data even once the system is shut off, and applies the correct offsets to the system in order to retain the position of the motor over multiple turns.

   For example, if the Motor moves 5000 counts in the positive direction, then the servo is on turn 2. Turn 1 is within 0-4096, Turn 2 is within 4096-8192, Turn 3 is within 8192-12,288 and so on. This can happen in both the positive and negative directions.
   The problem is that when the system is shut down, the memory of the Dynamixel is cleared. When it is turned back on, the Dynamixel is no longer at 5000 counts, but instead, it is on count 904, since the Dynamixel is incapable of keeping track of multiple turns.
   It is at count 904 since it can only keep track of a number between 0 and 4096 due to the absolute encoder. The MRAM on the other hand keeps track of these turns by storing the position of the Dynamixel whenever it is updated. Once the system is started,
   the Arduino reads the last known position from the MRAM, and applies the correct offset to the Dynamixel with some simple calculations. This allows the motor to keep track of it's position over a wide range of turns, both in the negative and positive directions.
*/

// ********************************************************************************************************************************************************************************************
// Dymamixel Selection
// ********************************************************************************************************************************************************************************************

// Choose which kind of Dynamixel this program is going to be used for. Makes a HUGE difference in a number of different ways.

//#define Dynamixel_MX // Dynamixel MX28, MX64, enables multi turn, position saving, position correction
#define Dynamixel_Pro // Dynamixel PM42-010-260-R, disables multi turn, position saving, position correction
//define Dynamixel_Y // Dynamixel YM070-210-R099-RH, has nothing called multi turn, saves using built in battery, has a lot more counts

// ********************************************************************************************************************************************************************************************
// Dependancies required for this code to function
// ********************************************************************************************************************************************************************************************

#include "setup.h"            // I use this to declare global variables, include libraries, define pins, 
#include "Serial_Functions.h" // All of the functions dealing with communications between the MCU, PC and Dynamixel
#include "EEPROM_Functions.h" // All of the functions that relate to the memory chip

// ********************************************************************************************************************************************************************************************
// Variables
// ********************************************************************************************************************************************************************************************

int           PC_numBytes = 0;              // Number of bytes that are ACTUALLY at the serial port (PC)
int           PC_Expected_Bytes = 8;        // Number of bytes that SHOULD be at the serial port (PC)

unsigned long    previousMillis = 0;        // This will store last millis value
const long       interval = 100;            // interval at which to run functions (milliseconds)
// These are used to time things without having to resort to delay which pauses the program

void setup() {

  dxl.begin(57600);                // Initialize communications with the Dynamixel at 57600 baud, which is the default
  dxl.setPortProtocolVersion(2.0); // Set Port Protocol Version. This has to match with the DYNAMIXEL protocol version.
  PC_SERIAL.begin(115200);         // Initialize communications with the PC at 115200 baud
  while(!PC_SERIAL) delay(10);     // Only start sending data 10ms after the serial port is open

  //PC_SERIAL.println("Serial Communications Established");

  if (fram.begin()) // Initializes the MRAM, also checks to see if it is present.
  {
    // PC_SERIAL.println(F("Found SPI MRAM \n"));
  }
  else
  {
    // PC_SERIAL.println(F("No SPI MRAM found ... check your connections\r\n")); // This no longer does anything because the MRAM does not support this check
  }

  // ********************************************************************************************************************************************************************************************
  // Set Dynamixel defaults
  // ********************************************************************************************************************************************************************************************

  if (dxl.ping(DXL_ID) == false) // If the Dynamixel is not connected, report it and run the Fault_Condition function.
  {
    PC_SERIAL.println(F("Ping error detected in Setup Function"));
    Fault = true; // Set the fault status to true
    Fault_Condition(); // Run the fault condition function
  }

  dxl.torqueOff(DXL_ID); // Turn the Torque Off. Prerequisite for changing EEPROM data.

  // Check current Dynamixel operating mode and change if necessary
  // Mode 3 = OP_POSITION = Position Control Mode - One full rotation =
  // -263,187 to 263,187) (Actually -262,931 to 262,931) for DXL Pro
  // -25,952,256 to 25,952,256 for DXL Y
  // 0-4096 for DXL MX
  // Mode 4 = OP_EXTENDED_POSITION = Extended Position Control Mode (Multi Turn Mode from -2,147,483,648 to 2,147,483,647 (DXL Pro) or -1,048,575 to 1,048,575 (DXL MX))
  
  #ifdef Dynamixel_MX
  while (dxl.readControlTableItem(OPERATING_MODE, DXL_ID) != 4)
  {
    dxl.setOperatingMode(DXL_ID, OP_EXTENDED_POSITION);
    PC_SERIAL.print(F("Operating Mode changed"));
  };
  #endif

  #ifdef Dynamixel_Pro
  while (dxl.readControlTableItem(OPERATING_MODE, DXL_ID) != 3)
  {
    dxl.setOperatingMode(DXL_ID, OP_POSITION);
    PC_SERIAL.print(F("Operating Mode changed"));
  };
  #endif

  #ifdef Dynamixel_Y
  while (dxl.readControlTableItem(OPERATING_MODE, DXL_ID) != 3)
  {
    dxl.setOperatingMode(DXL_ID, OP_POSITION);
    PC_SERIAL.print(F("Operating Mode changed"));
  };
  #endif
  
  // Set Dynamixel Speed Limit (From 0 to 2600 for Dynamixel Pro, 0-1023 for MX Series, -200000 to 20000 for DXL Y Apparently)
  #ifdef Dynamixel_MX
  while (dxl.readControlTableItem(VELOCITY_LIMIT, DXL_ID) != 1023)
  {
    dxl.writeControlTableItem(VELOCITY_LIMIT, DXL_ID, 1023);
    PC_SERIAL.println(F("Speed setting changed"));
  };
  #endif

  #ifdef Dynamixel_Pro
   while (dxl.readControlTableItem(VELOCITY_LIMIT, DXL_ID) != 2600)
  {
    dxl.writeControlTableItem(VELOCITY_LIMIT, DXL_ID, 2600);
    PC_SERIAL.println(F("Speed setting changed"));
  };
  #endif

  #ifdef Dynamixel_Y
   while (dxl.readControlTableItem(GOAL_VELOCITY, DXL_ID) != 200000)
  {
    dxl.writeControlTableItem(GOAL_VELOCITY, DXL_ID, 200000);
    PC_SERIAL.println(F("Speed setting changed"));
  };
  #endif

  // Set the Positon P Gain (0 - 16,383) Default for MX64 is 850, Default for DXL Pro is 1061, Default for DXL Y is 6283185
  #ifdef Dynamixel_MX
  while (dxl.readControlTableItem(POSITION_P_GAIN, DXL_ID) != 850)
  {
    dxl.writeControlTableItem(POSITION_P_GAIN, DXL_ID, 850);
    //PC_SERIAL.println(F("P Gain setting Changed"));
  };
  #endif

  #ifdef Dynamixel_Pro
  while (dxl.readControlTableItem(POSITION_P_GAIN, DXL_ID) != 1061)
  {
    dxl.writeControlTableItem(POSITION_P_GAIN, DXL_ID, 1061);
    //PC_SERIAL.println(F("P Gain setting Changed"));
  };
  #endif

  #ifdef Dynamixel_Y
  while (dxl.readControlTableItem(POSITION_P_GAIN, DXL_ID) != 6283185)
  {
    dxl.writeControlTableItem(POSITION_P_GAIN, DXL_ID, 6283185);
    //PC_SERIAL.println(F("P Gain setting Changed"));
  };
  #endif


  // Reset the Multi Turn Offset. Protocol 2.0 calls this "Homing Offset", sometimes this gets stuck
  while (dxl.readControlTableItem(HOMING_OFFSET, DXL_ID) != 0)
  {
    dxl.writeControlTableItem(HOMING_OFFSET, DXL_ID, 0);
    //PC_SERIAL.println(F("Homing Offset reset to 0"));
  };

  // These settings are only required in Multi Turn Mode for the MX Series
  #ifdef Dynamixel_MX
  // Check to see where the Dynamixel is before any adjustments. Useful for debugging.
  Raw_Position = (dxl.readControlTableItem(PRESENT_POSITION, DXL_ID)); // Read the current position of the Dynamixel
 
  DXL_Offset = Load_Position(); // Run the Load_Position function to calculate the Dynamixel Offset
  long Constrained_DXL_Offset = constrain(DXL_Offset, -1044479, 1044479); // MX28 and MX64 can only receive positions between -1,044,479 and 1,044,479 for protocol 2.0

  // Write the stored Multi Turn Offset back to the Dynamixel. This is the new Multi Turn Offset
  // Protocol 2.0 calls this "Homing Offset" instead of "Multi Turn Offset"
  while (dxl.readControlTableItem(HOMING_OFFSET, DXL_ID) != Constrained_DXL_Offset)
  {
    dxl.writeControlTableItem(HOMING_OFFSET, DXL_ID, Constrained_DXL_Offset);
    //PC_SERIAL.print((String)"New offset after setting is " + dxl.readControlTableItem(HOMING_OFFSET, DXL_ID) + "\n");;
  };
  //Correct_Position(); // Run the Correct_Position function. This is used to handle the supposedly rare rollover bug.
  #endif

  //PC_SERIAL.println(F("Initial settings complete."));

  dxl.torqueOn(DXL_ID);  // Re-enable the torque
}

void loop() {

  // This is a timer loop. Instead of a delay() that interrupts the program, this allows multiple things to happen in "parallel".
  unsigned long currentMillis = millis(); // Get the current time in ms that the program has been running

  if (currentMillis - previousMillis >= interval) // This will check to see what the current time of the program is. If a certain time has passed, reset the timer, and run the Save_Position function
  {
    previousMillis = currentMillis; // Reset the loop timer
    #ifdef Dynamixel_MX
    Save_Position(); // Runs the save position function. This constantly compares the turn saved in the MRAM to the actual turn that the Dynamixels are on.
    #endif
  }

  PC_numBytes = PC_SERIAL.available(); // Check to see how many bytes are waiting at the serial port.

  if (PC_numBytes >= 1) // If there is something at the serial port:
  {
    // Peek to see if first character is the dollar sign. If not, flush the RX buffer because it's not important. The $ character is what I use to send valid commands.
    if (PC_SERIAL.peek() != '$')
    {
      while (PC_SERIAL.available() > 0) PC_SERIAL.read(); // Flush serial RX buffer by reading the data
    }
  }

  if (PC_numBytes >= PC_Expected_Bytes) // If there are the correct number of bytes waiting, then:
  {
    Serial_Parse(PC_Expected_Bytes);  // run the Serial_Parse Function
  }

}