// ********************************************************************************************************************************************************************************************
// This handles the outgoing data to the PC serial port
// ********************************************************************************************************************************************************************************************

void Serial_Respond() // Responds to the PC with all of the various pieces of data.
{
  long    Present_Position = 0;         // Contains the Dynamixel's present position
  char    Present_Position_Bytes[5];    // Contains the Dynamixel's present position as an array of 4 Bytes
  long    Goal_Position = 0;            // Contains the Dynamixel's Goal position
  char    Goal_Position_Bytes[5];       // Contains the Dynamixel's Goal position as an array of 4 Bytes 
  byte    Moving;                       // Holds the status of the Dynamixel, either moving or not
  byte    Error;                        // Contains any errors producted by the Dynamixel

  // Get Dynamixel positions
  Present_Position = (dxl.readControlTableItem(PRESENT_POSITION, DXL_ID)); // Read the present position
  // Convert the 32-Bit number to an array of 4 Bytes
  Present_Position_Bytes[0] = Present_Position & 0xFF;
  Present_Position_Bytes[1] = (Present_Position >> 8) & 0xFF;
  Present_Position_Bytes[2] = (Present_Position >> 16) & 0xFF;
  Present_Position_Bytes[3] = (Present_Position >> 24) & 0xFF;
  Goal_Position = (dxl.readControlTableItem(GOAL_POSITION, DXL_ID)); // Read the goal position
  // Convert the 32-Bit number to an array of 4 Bytes
  Goal_Position_Bytes[0] = Goal_Position & 0xFF;
  Goal_Position_Bytes[1] = (Goal_Position >> 8) & 0xFF;
  Goal_Position_Bytes[2] = (Goal_Position >> 16) & 0xFF;
  Goal_Position_Bytes[3] = (Goal_Position >> 24) & 0xFF;
  Moving = (dxl.readControlTableItem(MOVING, DXL_ID)); // Check to see if the Dynamixel is moving
  Error = dxl.readControlTableItem(HARDWARE_ERROR_STATUS, DXL_ID); // Read the Hardware Error Status
  //Error = (dxl.getLastLibErrCode()); // Check what errors have been reported
  
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
  long        Desired_Position = 0;           // The desired position of the Dynamixel, either valid or invalid
  long        Goal_Position = 0;              // The goal position value of the Dynamixel
  long        Valid_Position = 0;             // The desired position after it has been constrained within the limits, making it a valid position to move to
  
  // Parse received serial
  for (int x=0; x < Bytes; x++) //Put each byte received into an array
  {
    PC_Rx_Sentence[x] = PC_SERIAL.read();
  }

  // If the command starts with the $ sign, ends with the # sign, and the checksum matches (Same checksum style as Dynamixel) do this:
  if (PC_Rx_Sentence[0] == '$' && PC_Rx_Sentence[Bytes - 1] == '#' && PC_Rx_Sentence[5] == lowByte(~(PC_Rx_Sentence[1] + PC_Rx_Sentence[2] + PC_Rx_Sentence[3] + PC_Rx_Sentence[4])) && PC_Rx_Sentence[6] == '%')
    {    
        Desired_Position = (PC_Rx_Sentence[1] << 24) | (PC_Rx_Sentence[2] << 16) | ( PC_Rx_Sentence[3] << 8 ) | (PC_Rx_Sentence[4]); // This is how you make a 32-Bit number with four bytes, two high bytes and two low bytes.

        #ifdef Dynamixel_MX
        Valid_Position = constrain(Desired_Position, -19500, 13000); // Put the desired position into the valid position after it has been constrained (For VM200)
        //Valid_Position = constrain(Desired_Position, -1044479, 1044479); // Put the desired position into the valid position after it has been constrained (For general use)
        Goal_Position = (dxl.readControlTableItem(GOAL_POSITION, DXL_ID)); // Read the current goal position from the Dynamixel
        #endif

        #ifdef Dynamixel_Pro
        Valid_Position = Desired_Position; // No need to constrain for DXL Pro
        Goal_Position = (dxl.readControlTableItem(GOAL_POSITION, DXL_ID)); // Read the current goal position from the Dynamixel
        #endif

        if (Valid_Position == Goal_Position) // If the valid position is the same as the goal position, don't write anything, as nothing has changed.
        {
          Serial_Respond(); // Respond to PC
        }

        else if (Valid_Position != Goal_Position) // If the valid position is not the same as the goal position:
        {
          dxl.setGoalPosition(DXL_ID, Desired_Position); // Set the new goal position
          Serial_Respond(); // Respond to PC
        }
     }
       
        
  // If the arduino receives the specific command $000000# then respond with this:
  else if (PC_Rx_Sentence[0] == '$' && PC_Rx_Sentence[Bytes - 1] == '#' && PC_Rx_Sentence[1] == '0' && PC_Rx_Sentence[2] == '0' && PC_Rx_Sentence[3] == '0' && PC_Rx_Sentence[4] == '0' && PC_Rx_Sentence[5] == '0' && PC_Rx_Sentence[6] == '0')
  {
      PC_SERIAL.print(F("VM200G")); // This is to let the PC know that it has found the right device 
  }
  
  // If the arduino receives the specific command $DEBUG!# then respond with this:
  else if (PC_Rx_Sentence[0] == '$' && PC_Rx_Sentence[Bytes - 1] == '#' && PC_Rx_Sentence[1] == 'D' && PC_Rx_Sentence[2] == 'E' && PC_Rx_Sentence[3] == 'B' && PC_Rx_Sentence[4] == 'U' && PC_Rx_Sentence[5] == 'G' && PC_Rx_Sentence[6] == '!')
  {
    char       Stored_Pos[5];          // An array of 4 bytes that contains the Dynamixel's last known position
    Stored_Pos[0] = fram.read8(0x0);
    Stored_Pos[1] = fram.read8(0x1);
    Stored_Pos[2] = fram.read8(0x2);
    Stored_Pos[3] = fram.read8(0x3);

    long Calculated_Stored_Pos = (Stored_Pos[0] << 24) | (Stored_Pos[1] << 16) | (Stored_Pos[2] << 8 ) | (Stored_Pos[3]); // This is how to combine 4 bytes into a 32-Bit Number
  
    float Stored_Turn = Calculated_Stored_Pos / 4096.00; // Divide the stored position by 4096 to get the turn number. Decimals need to be added for float math.
    long Abs_Stored_Turn = floor(Stored_Turn); // Gets rid of the decimals on the turn number by rounding down, and store as Abs_Present_Turn

    PC_SERIAL.println(); // Just a blank line for readability
    PC_SERIAL.print(F("Current Dynamixel ID is "));
    PC_SERIAL.println(dxl.readControlTableItem(ID, DXL_ID)); // Read the values
    PC_SERIAL.print(F("Dynamixel speed is set to "));
    PC_SERIAL.println(dxl.readControlTableItem(VELOCITY_LIMIT, DXL_ID)); // Read the values
    PC_SERIAL.print(F("Dynamixel position P Gain is set to "));
    PC_SERIAL.println(dxl.readControlTableItem(POSITION_P_GAIN, DXL_ID)); // Read the values
    PC_SERIAL.print(F("Last known position was "));
    PC_SERIAL.println(Last_Pos);
    PC_SERIAL.print(F("Saved position is "));
    PC_SERIAL.println(Calculated_Stored_Pos);
    PC_SERIAL.print(F("Present position is "));
    PC_SERIAL.println(dxl.readControlTableItem(PRESENT_POSITION, DXL_ID)); // Read the values
    PC_SERIAL.print(F("Raw Dynamixel Position with no offset was "));
    PC_SERIAL.println(Raw_Position);
    PC_SERIAL.print(F("Dynamixel current offset is set to "));
    PC_SERIAL.println(dxl.readControlTableItem(HOMING_OFFSET, DXL_ID)); // Read the values
    PC_SERIAL.print(F("Current saved Turn is "));
    PC_SERIAL.println(Abs_Stored_Turn);
    PC_SERIAL.print(F("Last Reported Error was "));
    PC_SERIAL.println((dxl.getLastLibErrCode()));
    PC_SERIAL.print(F("Current Hardware Error is "));
    PC_SERIAL.println(dxl.readControlTableItem(HARDWARE_ERROR_STATUS, DXL_ID)); // Read the values
    PC_SERIAL.println(F("Current controller firmware is version 2.4 - Built June 13th 2024"));
    #ifdef Dynamixel_MX
    PC_SERIAL.print(F("Current firmware is for Dynamixel MX"));  
    #endif
    #ifdef Dynamixel_Pro
    PC_SERIAL.print(F("Current firmware is for Dynamixel Pro"));  
    #endif
  }
  
  else if (PC_Rx_Sentence[0] == '$' && PC_Rx_Sentence[Bytes - 1] == '#' && PC_Rx_Sentence[1] == '1' && PC_Rx_Sentence[2] == '2' && PC_Rx_Sentence[3] == '3' && PC_Rx_Sentence[4] == '4')
  {
    // If the arduino receives the specific command $123400# then just respond to the PC:
    Serial_Respond(); // Respond to PC   
  } 
  
}