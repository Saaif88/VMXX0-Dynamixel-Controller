// ********************************************************************************************************************************************************************************************
// This handles the outgoing data to the PC serial port
// ********************************************************************************************************************************************************************************************

void Serial_Respond() // Responds to the PC with all of the various pieces of data. This can be changed, but this is what all the LabView software uses so far
{
  long    Present_Position = 0;         // Contains the Dynamixel's present position
  char    Present_Position_Bytes[5];    // Contains the Dynamixel's present position as an array of 4 Bytes
  long    Goal_Position = 0;            // Contains the Dynamixel's Goal position
  char    Goal_Position_Bytes[5];       // Contains the Dynamixel's Goal position as an array of 4 Bytes
  char    Requested_Position_Bytes[5];  // The requested positon of the Dynamixel, sent from the PC as an array of 4 Bytes   
  byte    Moving;                       // Holds the status of the Dynamixel, either moving or not
  byte    Error;                        // Contains any errors producted by the Dynamixel

  // Get Dynamixel positions
  Present_Position = (dxl.readControlTableItem(PRESENT_POSITION, DXL_ID)); // Read the present position
  // Convert the 32-Bit number to an array of 4 Bytes
  Present_Position_Bytes[0] = Present_Position;
  Present_Position_Bytes[1] = Present_Position >> 8;
  Present_Position_Bytes[2] = Present_Position >> 16;
  Present_Position_Bytes[3] = Present_Position >> 24;
  Goal_Position = (dxl.readControlTableItem(GOAL_POSITION, DXL_ID)); // Read the goal position
  // Convert the 32-Bit number to an array of 4 Bytes
  Goal_Position_Bytes[0] = Goal_Position;
  Goal_Position_Bytes[1] = Goal_Position >> 8;
  Goal_Position_Bytes[2] = Goal_Position >> 16;
  Goal_Position_Bytes[3] = Goal_Position >> 24;
  Requested_Position_Bytes[0] = Requested_Position;
  Requested_Position_Bytes[1] = Requested_Position >> 8;
  Requested_Position_Bytes[2] = Requested_Position >> 16;
  Requested_Position_Bytes[3] = Requested_Position >> 24;
  Moving = (dxl.readControlTableItem(MOVING, DXL_ID)); // Check to see if the Dynamixel is moving
  Error = (dxl.getLastLibErrCode()); // Check what errors have been reported
  
  // Response
   PC_SERIAL.write('$'); // This is the starting character so that the PC knows where to start reading
   PC_SERIAL.write(Goal_Position_Bytes[3]); // 
   PC_SERIAL.write(Goal_Position_Bytes[2]); // 
   PC_SERIAL.write(Goal_Position_Bytes[1]); // 
   PC_SERIAL.write(Goal_Position_Bytes[0]); // 
   PC_SERIAL.write(Present_Position_Bytes[3]); // 
   PC_SERIAL.write(Present_Position_Bytes[2]); // 
   PC_SERIAL.write(Present_Position_Bytes[1]); // 
   PC_SERIAL.write(Present_Position_Bytes[0]); //
   PC_SERIAL.write(Requested_Position_Bytes[3]); // 
   PC_SERIAL.write(Requested_Position_Bytes[2]); // 
   PC_SERIAL.write(Requested_Position_Bytes[1]); // 
   PC_SERIAL.write(Requested_Position_Bytes[0]); //
   PC_SERIAL.write(Moving); // Send the moving status
   PC_SERIAL.write(Error); // Send the error byte
   //PC_SERIAL.write(lowByte(~(lowByte(Goal_Position) + highByte(Goal_Position) + lowByte(Present_Position) + highByte(Present_Position) + Moving + Error))); // Send the checksum
   PC_SERIAL.write('#'); // This is the last character so that the PC knows when to stop reading
}

// ********************************************************************************************************************************************************************************************
// This handles the incoming data from the PC serial port
// ********************************************************************************************************************************************************************************************

void Serial_Parse(int Bytes)
{
  char        PC_Rx_Sentence[9] = "$000000#";  // Initialize received serial sentence (PC, 8 bytes)
  // Why 9? Because C expects an extra "null byte" when it comes to char arrays apparently. I could have just left this blank, but this seems important to remember.
  //long        Desired_Position = 0;           // The desired position of the Dynamixel, either valid or invalid
  long        Goal_Position = 0;              // The goal position value of the Dynamixel
  long        Valid_Position = 0;             // The desired position after it has been constrained within the limits, making it a valid position to move to
  //long        Low_Limit = -1044479;           // The lowest allowable position (Min -1,044,479 for MX28/MX64, -2,147,483,648 for DXL Pro)
  //long        High_Limit = 1044479;           // The highest allowable position (Max 1,044,479 for MX28/MX64,  2,147,483,648 for DXL Pro)
  
  long        DXL_Stored_Offset = 0;            // Holds the Dynamixel's Stored Offset (In MRAM)
  long        Original_Offset = 0;              // Holds the Original Offset
  long        New_Offset = 0;                   // Holds the New Offset
  long        Last_Pos = 0;
  long        Abs_Present_Turn = 0;
  
  
  // Parse received serial
  for (int x=0; x < Bytes; x++) //Put each byte received into an array
  {
    PC_Rx_Sentence[x] = PC_SERIAL.read();
  }

  // If the command starts with the $ sign, ends with the # sign, and the checksum matches (Same checksum style as Dynamixel) do this:
  if (PC_Rx_Sentence[0] == '$' && PC_Rx_Sentence[Bytes - 1] == '#' && PC_Rx_Sentence[5] == lowByte(~(PC_Rx_Sentence[1] + PC_Rx_Sentence[2] + PC_Rx_Sentence[3] + PC_Rx_Sentence[4])) && PC_Rx_Sentence[6] == '%')
    {    
        Requested_Position = (PC_Rx_Sentence[1] << 24) | (PC_Rx_Sentence[2] << 16) | ( PC_Rx_Sentence[3] << 8 ) | (PC_Rx_Sentence[4]);
        // This is how you make a 32-Bit number with four bytes, two high bytes and two low bytes.
          Valid_Position = Requested_Position;

        //Valid_Position = constrain(Desired_Position, Low_Limit, High_Limit); // Put the desired position into the valid position after it has been constrained if necessary
        Goal_Position = (dxl.readControlTableItem(GOAL_POSITION, DXL_ID)); // Read the current goal position from the Dynamixel

        if (Valid_Position == Goal_Position) // If the valid position is the same as the goal position, don't write anything, as nothing has changed.
        {
          Serial_Respond(); // Respond to PC
        }

        else if (Valid_Position != Goal_Position) // If the valid position is not the same as the goal position:
        {
          dxl.setGoalPosition(DXL_ID, Valid_Position); // Set the new goal position
          //dxl.setGoalPosition(DXL_ID, Requested_Position); // Set the new goal position 
          Serial_Respond(); // Respond to PC
        }
     }
       
        
  // If the arduino receives the specific command $000000# then respond with this:
  else if (PC_Rx_Sentence[0] == '$' && PC_Rx_Sentence[Bytes - 1] == '#' && PC_Rx_Sentence[1] == '0' && PC_Rx_Sentence[2] == '0' && PC_Rx_Sentence[3] == '0' && PC_Rx_Sentence[4] == '0' && PC_Rx_Sentence[5] == '0' && PC_Rx_Sentence[6] == '0')
  {
      PC_SERIAL.print(F("VM200G")); // This is to let the PC know that it has found the right device 
  }
  
  // If the arduino receives the specific command $DEBUG# then respond with this:
  // This is essentially a debug command
  else if (PC_Rx_Sentence[0] == '$' && PC_Rx_Sentence[Bytes - 1] == '#' && PC_Rx_Sentence[1] == 'D' && PC_Rx_Sentence[2] == 'E' && PC_Rx_Sentence[3] == 'B' && PC_Rx_Sentence[4] == 'U' && PC_Rx_Sentence[5] == 'G' && PC_Rx_Sentence[6] == '!')
  {
     PC_SERIAL.println(); // Just a blank line for readability
     PC_SERIAL.print(F("Current Dynamixel ID is "));
     PC_SERIAL.println(dxl.readControlTableItem(ID, DXL_ID)); // Read the values
     PC_SERIAL.print(F("Dynamixel Speed is set to "));
     PC_SERIAL.println(dxl.readControlTableItem(MOVING_SPEED, DXL_ID)); // Read the values
     PC_SERIAL.print(F("Dynamixel Torque is set to "));
     PC_SERIAL.println(dxl.readControlTableItem(MAX_TORQUE, DXL_ID)); // Read the values
     PC_SERIAL.print(F("Dynamixel P Gain is set to "));
     PC_SERIAL.println(dxl.readControlTableItem(P_GAIN, DXL_ID)); // Read the values
     PC_SERIAL.print(F("Last Known Position "));
     PC_SERIAL.println(Last_Pos);
     PC_SERIAL.print(F("Present Position "));
     PC_SERIAL.println(dxl.readControlTableItem(PRESENT_POSITION, DXL_ID)); // Read the values
     PC_SERIAL.print(F("Original Offset was: "));
     PC_SERIAL.println(Original_Offset);
     PC_SERIAL.print(F("New Offset after setting is: "));
     PC_SERIAL.println(New_Offset);
     PC_SERIAL.print(F("Dynamixel Current Offset set to "));
     PC_SERIAL.println(dxl.readControlTableItem(MULTI_TURN_OFFSET, DXL_ID)); // Read the values
     PC_SERIAL.print(F("Dynamixel Stored Offset "));
     PC_SERIAL.println(DXL_Stored_Offset);
     PC_SERIAL.print(F("Current Saved Turn is "));
     PC_SERIAL.println(Abs_Present_Turn);
     //PC_SERIAL.print(F("System has corrected itself in the positive direction "));
     //PC_SERIAL.print(fram.read8(0x2));
     //PC_SERIAL.println(F(" times"));
     //PC_SERIAL.print(F("System has corrected itself in the negative direction "));
     //PC_SERIAL.print(fram.read8(0x3));
     //PC_SERIAL.println(F(" times"));
     PC_SERIAL.print(F("Current controller firmware is version 2.2 "));
  }
  
  else if (PC_Rx_Sentence[0] == '$' && PC_Rx_Sentence[Bytes - 1] == '#' && PC_Rx_Sentence[1] == '1' && PC_Rx_Sentence[2] == '2' && PC_Rx_Sentence[3] == '3' && PC_Rx_Sentence[4] == '4')
  {
    // If the arduino receives the specific command $123400# then just respond to the PC:
    Serial_Respond(); // Respond to PC   
  } 
  
}