// ********************************************************************************************************************************************************************************************
// This puts the system into a fault condition where it will not do anything at all until the system is power cycled.
// ********************************************************************************************************************************************************************************************
void Fault_Condition()
{
  while(Fault == true)
  {
     PC_SERIAL.println(); // Just a blank line for readability
     PC_SERIAL.println((String)"ERROR! Could not find a Dynamixel with ID " + DXL_ID);
     PC_SERIAL.print(F("Please turn everything off and check your connections!"));
     delay(500);
  }
}

// ********************************************************************************************************************************************************************************************
// This puts the present Dynamixel's Turn into the Arduino's EEPROM for Storage
// ********************************************************************************************************************************************************************************************
void Save_Position() // Does what it says, saves the present position of the Dynamixel to the MRAM
  {
    long       Current_Position;             // Contains the Dynamixel's current position
    byte       Current_Position_Bytes[4];    // Contains the Dynamixel's current position as a collection of bytes
    long       Calculated_Stored_Pos;        // The stored position, created from the 4 saved bytes in the MRAM  
    byte       Stored_Pos[4];                // The stored position as individual bytes
    long       Pos_Difference;               // The difference between the stored position and the actual position
    bool       Pos_Changed;                  // A True/False that holds whether the position has changed or not

    // Uses the stored bytes to calculate the last known position. This is done because the memory can only store bytes, not words (Aka I don't know how to make it do that)
    Stored_Pos[0] = fram.read8(0x0);
    Stored_Pos[1] = fram.read8(0x1);
    Stored_Pos[2] = fram.read8(0x2);
    Stored_Pos[3] = fram.read8(0x3);

    Calculated_Stored_Pos = (Stored_Pos[0] << 24) | (Stored_Pos[1] << 16) | (Stored_Pos[2] << 8 ) | (Stored_Pos[3]); // This is how to combine 4 bytes into a 32-Bit Number
    
    Current_Position = (dxl.readControlTableItem(PRESENT_POSITION, DXL_ID)); // Read the current position of the Dynamixel, and store it temporarily

    Pos_Difference = Calculated_Stored_Pos - Current_Position; // The position difference is just the stored position minus the present position
    
    //PC_SERIAL.println((String)"Difference between current and stored position is " + Pos_Difference);

   if (Pos_Difference > 1 || Pos_Difference < -1) // If the position is different by more than one point in either the positive or the negative direction:
    {
      Pos_Changed = true; // The position has changed
      //PC_SERIAL.println((String)"Position is different by " + Pos_Changed + "counts");
    }
   else
    {
      Pos_Changed = false; // If there is no difference, then the position has not changed
      // PC_SERIAL.println(F("Position is the same \n"));
    }   

    
  if (dxl.ping(DXL_ID) == false) // If the Dynamixel is not connected, report it and run the Fault_Condition function.
    {
      PC_SERIAL.println(F("Ping error detected in Save_Position Function"));
      Fault = true; // Set the fault status to true
      Fault_Condition(); // Run the fault condition function
    }

  else if (Pos_Changed == true && dxl.ping(DXL_ID) == true) // If the stored position of the Dynamixel is NOT the same as the current position and the Dynamixel is connected
    { 
       Current_Position = (dxl.readControlTableItem(PRESENT_POSITION, DXL_ID)); // Read the current position of the Dynamixel, and store it as bytes      
       Current_Position_Bytes[0] = Current_Position >> 24;
       Current_Position_Bytes[1] = Current_Position >> 16;
       Current_Position_Bytes[2] = Current_Position >> 8;
       Current_Position_Bytes[3] = Current_Position;


      fram.writeEnable(true); // Enable writing 
      fram.write8(0x0, Current_Position_Bytes[0]); // Write the first byte to memory location 0x0
      fram.writeEnable(false); //Stop writing because we are done writing to that location
      fram.writeEnable(true); //Enable writing 
      fram.write8(0x1, Current_Position_Bytes[1]); // Write the second byte to memory location 0x1
      fram.writeEnable(false); //Stop writing because we are done writing to that location
      fram.writeEnable(true); //Enable writing 
      fram.write8(0x2, Current_Position_Bytes[2]); // Write the third byte to memory location 0x2
      fram.writeEnable(false); //Stop writing because we are done writing to that location
      fram.writeEnable(true); //Enable writing 
      fram.write8(0x3, Current_Position_Bytes[3]); // Write the fourth byte to memory location 0x3
      fram.writeEnable(false); //Stop writing because we are done writing to that location

      // PC_SERIAL.println(F("Dynamixel Current Position Saved to MRAM \n"));
    }

    else if (Pos_Changed == false) // If the positon has not changed, then there is no reason to save it. This avoid wearing out the MRAM (Even if it is infinite...)
    {
      // PC_SERIAL.println(F("MRAM Matches Current Position for Dynamixel, No Change \n"));
    }  
     
     // PC_SERIAL.print((String)"Dynamixel Current Position is " + Current_Position + '\n');
     // PC_SERIAL.print((String)"Dynamixel Current Stored Position is " + Calculated_Stored_Pos + '\n');
    
  }
  
// ********************************************************************************************************************************************************************************************
// This function reads the stored values in the MRAM and uses it to calculate the Dynamixel's Multi Turn Offset
// ********************************************************************************************************************************************************************************************

long Load_Position()
{
  long       MT_Offset;                    // Stores the multi turn offset of the Dynamixel
  float      Stored_Turn;                  // Stores the current turn number with decimals
  long       Abs_Stored_Turn;              // Stores the current turn number without decimals, rounded down
  byte       Stored_Pos[4];                // An array of 4 bytes that contains the Dynamixel's last known position
  long       Calculated_Stored_Pos;        // The Dynamixel's last position created from the stored bytes

  // Reads the 4 saved bytes and puts them each into an array
  Stored_Pos[0] = fram.read8(0x0);
  Stored_Pos[1] = fram.read8(0x1);
  Stored_Pos[2] = fram.read8(0x2);
  Stored_Pos[3] = fram.read8(0x3);

  Calculated_Stored_Pos = (Stored_Pos[0] << 24) | (Stored_Pos[1] << 16) | (Stored_Pos[2] << 8 ) | (Stored_Pos[3]); // This is how to combine 4 bytes into a 32-Bit Number
  
  Stored_Turn = Calculated_Stored_Pos / 4096.00; // Divide the stored position by 4096 to get the turn number. Decimals need to be added for float math.
  Abs_Stored_Turn = floor(Stored_Turn); // Gets rid of the decimals on the turn number by rounding down, and store as Abs_Present_Turn
  
  MT_Offset = (Abs_Stored_Turn * 4096); // Multiply the saved turn by 4096 to get the Multi Turn offset value
  
   PC_SERIAL.println((String)"The Current Stored Position of the Dynamixel is " + Calculated_Stored_Pos);
   PC_SERIAL.println((String)"The Current Stored Turn of the Dynamixel is " + Abs_Stored_Turn);
   PC_SERIAL.println((String)"The new multi turn offset of the Dynamixel is " + MT_Offset);

  return MT_Offset; // Return the new calculated number
}

// ********************************************************************************************************************************************************************************************
// This function compares the stored Dynamixel position to the current new Dynamixel position and corrects it if there are any anomalies
// ********************************************************************************************************************************************************************************************

void Correct_Position()
{
  
  bool         Ping1;                       // Used the hold the status of the 1st Dynamixel (Either present or not)
  byte         Stored_Pos_highbyte;         // Contains the Dynamixel's last position highbyte that is stored in the EEPROM
  byte         Stored_Pos_lowbyte;          // Contains the Dynamixel's last position lowbyte that is stored in the EEPROM
  word         Stored_Pos_word;             // Contains the Dynamixel's last position that is stored in the MRAM (Combination of the lowbyte and highbyte)
  short        Stored_Pos;                  // Contains the Dynamixel's last position that is stored in the MRAM as a short because the position can be either negative or positive.
  short        Present_Position;            // Contains the Dynamixel's Present Position
  short        Position_Difference;         // Contains the difference between the present positon and stored positions
  word         abs_Position_Difference;     // Contains the difference without negative numbers
  short        Current_Offset;              // Contains the current offset of the Dynamixel
  short        New_Offset;                  // Contains the new offset to be sent to the Dynamixel
  byte         Pos_Corrected_Count = 0;     // the stored number of positive corrections the system has made
  byte         Neg_Corrected_Count = 0;     // the stored number of negative corrections the system has made

  Ping1 = (dxl.ping(DXL_ID)); // Ping the Dynamixel to see if it is alive/connected
  delay(5);

  Stored_Pos_highbyte = fram.read8(0x0); // Read the stored highbyte of the last known position from the fram
  Stored_Pos_lowbyte = fram.read8(0x1); // Read the stored lowbyte of the last known position from the fram
  Stored_Pos_word = word(Stored_Pos_highbyte, Stored_Pos_lowbyte); // Combine the highbyte and lowbyte into the actual position
  Stored_Pos = (short) Stored_Pos_word; // Cast the word to a short, since the position can be either negative or positive

  Pos_Corrected_Count = fram.read8(0x2); // Read the stored number of positive corrections the system has made
  Neg_Corrected_Count = fram.read8(0x3); // Read the stored number of negative corrections the system has made
  
  Present_Position = (dxl.readControlTableItem(PRESENT_POSITION, DXL_ID)); // Read the present position of the Dynamixel
  // PC_SERIAL.println(); // Just a blank line for readability
  // PC_SERIAL.print(F("Dynamixel Saved Position was "));
  // PC_SERIAL.println(Stored_Pos);
  // PC_SERIAL.print(F("Dynamixel Current Position is "));
  // PC_SERIAL.println(Present_Position);
  delay(5);

  Position_Difference = Present_Position - Stored_Pos; // Calculate the difference between the stored and present position
  abs_Position_Difference = abs(Position_Difference); // Get rid of any negative numbers
  // PC_SERIAL.println(); // Just a blank line for readability
  // PC_SERIAL.print(F("Position Difference is "));
  // PC_SERIAL.println(Position_Difference);
  // PC_SERIAL.print(F("Absolute Position Difference is "));
  // PC_SERIAL.println(abs_Position_Difference);

  
  if (Ping1 == false) // First, check to see if the Dynamixel is connected before making any corrections. If it is not connected:
  {
    Serial.println(); // Just a blank line for readability
    Serial.print(F("Ping Error Detected in Correct_Position function"));
    Fault = true; // Set the fault status to true
    Fault_Condition(); // Run the fault condition function
  }

  else if (Ping1 == true && Position_Difference < 0) // If the Dynamixel is connected and the position difference is a negative number:
  {
       // PC_SERIAL.println(); // Just a blank line for readability
       // PC_SERIAL.println(F("Negative Number detected"));

       if (abs_Position_Difference > 1000) // If the difference is more than 1000 counts
       {
          Current_Offset = (dxl.readControlTableItem(MULTI_TURN_OFFSET, DXL_ID)); // Read the current multi turn offset
          // PC_SERIAL.print(F("Position is different by > -1000 points"));
          // PC_SERIAL.println(); // Just a blank line for readability
          // PC_SERIAL.print(F("Current Offset is "));
          // PC_SERIAL.println (Current_Offset);
          New_Offset = Current_Offset + 4096; // The new offset will be the current offset plus 4096
          // PC_SERIAL.print(F("New Offset (Should be more) is "));
          // PC_SERIAL.println(New_Offset); 
          dxl.writeControlTableItem(MULTI_TURN_OFFSET, DXL_ID, New_Offset); // Write the new, correct offset to the Dynamixel
          
          fram.writeEnable(true); // Enable writing 
          fram.write8(0x2, Pos_Corrected_Count + 1); // Increment the number of corrections by 1
          fram.writeEnable(false); // Stop writing because we are done writing to that location
      }
      
      else // If the position has changed by less than 1000 points (abs_Position_Difference < 1000)
      {   
          // PC_SERIAL.println(); // Just a blank line for readability
          // PC_SERIAL.print(F("Position difference <1000, No Correction Required "));
      }
  }


  else if (Ping1 == true && Position_Difference > 0) // If the Dynamixel is connected and the position difference is a positive number:
  {
       // PC_SERIAL.println(); // Just a blank line for readability
       // PC_SERIAL.println(F("Positive Number detected"));

       if (abs_Position_Difference > 1000) // If the difference is more than 1000 counts
       {
          Current_Offset = (dxl.readControlTableItem(MULTI_TURN_OFFSET, DXL_ID)); // Read the current multi turn offset
          // PC_SERIAL.print(F("Position is different by > 1000 points"));
          // PC_SERIAL.println(); // Just a blank line for readability
          // PC_SERIAL.print(F("Current Offset is "));
          // PC_SERIAL.println (Current_Offset);
          New_Offset = Current_Offset - 4096;  // The new offset will be the current offset minus 4096
          // PC_SERIAL.print(F("New Offset (Should be less) is "));
          // PC_SERIAL.println (New_Offset); 
          dxl.writeControlTableItem(MULTI_TURN_OFFSET, DXL_ID, New_Offset);

          fram.writeEnable(true); // Enable writing 
          fram.write8(0x3, Neg_Corrected_Count + 1); // Increment the number of corrections by 1
          fram.writeEnable(false); //Stop writing because we are done writing to that location
       }
   
       else // If the position has changed by less than 1000 points (abs_Position_Difference < 1000)
       {
          // PC_SERIAL.println(); // Just a blank line for readability
          // PC_SERIAL.print(F("No Correction Required "));
       }
  }  
}