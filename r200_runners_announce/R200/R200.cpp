#include <Arduino.h>
#include "R200.h"

// Constructor
R200::R200() {}

// Initialize the RFID module with the given serial stream
bool R200::begin(Stream *serial){
  _serial = serial;
  return true;
}

// Utility functions for debugging - printHexByte, printHexBytes, printHexWord
void printHexByte(const char* name, uint8_t value){
  Serial.print(name);
  Serial.print(": ");
  if (value < 0x10) {
    Serial.print("0x0");
  } else {
    Serial.print("0x");
  }
  Serial.println(value, HEX);
}

void printHexBytes(const char* name, uint8_t *value, uint8_t len){
  Serial.print(name);
  Serial.print(": 0x");
  for(int i=0; i<len; i++){
    if(value[i] < 0x10){
      Serial.print("0");
    }
    Serial.print(value[i], HEX);
  }
  Serial.println("");
}

void printHexWord(const char* name, uint8_t MSB, uint8_t LSB){
  Serial.print(name);
  Serial.print(": ");
  if (MSB < 0x10) {
    Serial.print("0x0");
  } else {
    Serial.print("0x");
  }
  Serial.print(MSB, HEX);
  Serial.print(" ");
  if (LSB < 0x10) {
    Serial.print("0x0");
  } else {
    Serial.print("0x");
  }
  Serial.println(LSB, HEX);
}

// Main loop function to process incoming data
void R200::loop(){
  // Check if data is available from the RFID module
  if(dataAvailable()){
    // Attempt to receive a full frame of data with a timeout
    if(receiveData()){
      if(dataIsValid()){
        // If a full frame of data has been received and is valid, parse it
        switch(_buffer[R200_CommandPos]){
          case CMD_GetModuleInfo:
            for (uint8_t i = 0; i < RX_BUFFER_LENGTH - 8; i++) {
              Serial.print((char)_buffer[6 + i]);
              // Stop when only two bytes left are the CRC and FrameEnd marker
              if (_buffer[8 + i] == R200_FrameEnd) {
                break;
              }
            }
            Serial.println("");
            break;
          
          case CMD_SinglePollInstruction:
            // Example successful response:
            // AA 02 22 00 11 C7 30 00 E2 80 68 90 00 00 50 0E 88 C6 A4 A7 11 9B 29 DD 
            // Breakdown of response:
            // AA: Frame Header
            // 02: Instruction Code
            // 22: Command Parameter
            // 00 11: Instruction data length (0x11 = 17 bytes)
            // C7: RSSI Signal Strength
            // 30 00: Label PC code (factory reg code)
            // E2 80 68 90 00 00 50 0E 88 C6 A4 A7: EPC code
            // 11 9B: CRC check
            // 29: Verification
            // DD: End of frame

            #ifdef DEBUG
              printHexByte("RSSI", _buffer[6]);
              printHexWord("PC", _buffer[7], _buffer[8]);
              printHexBytes("EPC", &_buffer[9], 12);
            #endif

            if(memcmp(uid, &_buffer[9], 12) != 0) {
              memcpy(uid, &_buffer[9], 12);
              #ifdef DEBUG
                Serial.print("New card detected : ");
                dumpUIDToSerial();
                Serial.println("");
              #endif
            }
            else {
              #ifdef DEBUG
                Serial.print("Same card still present : ");
                dumpUIDToSerial();
                Serial.println("");
              #endif
            }

            #ifdef DEBUG
              printHexWord("CRC", _buffer[20], _buffer[21]);
            #endif
            break;
          
          case CMD_ExecutionFailure:
            switch(_buffer[R200_ParamPos]){
              case ERR_CommandError:
                Serial.println("Command error");
                break;
              
              case ERR_InventoryFail:
                // This is not necessarily a "failure" - it just means that there are no cards in range
                // If there was previously a UID
                if(memcmp(uid, blankUid, sizeof uid) != 0) {
                  #ifdef DEBUG
                    Serial.print("Card removed : ");
                    dumpUIDToSerial();
                    Serial.println("");
                  #endif
                  memset(uid, 0, sizeof uid);
                }
                break;
              
              case ERR_AccessFail:
                // Serial.println("Access Fail");
                break;
              
              case ERR_ReadFail:
                // Serial.println("Read fail");
                break;
              
              case ERR_WriteFail:
                // Serial.println("Write fail");
                break;
              
              default:
                // Serial.print("Fail code ");
                // Serial.println(_buffer[R200_ParamPos], HEX);
                break;
            }
            break;
          
          default:
            #ifdef DEBUG
              Serial.print("Unknown Command: ");
              Serial.println(_buffer[R200_CommandPos], HEX);
            #endif
            break;
        }
      }
    }
  }
}

// Check if data has been received and is valid
bool R200::dataIsValid(){
  // Calculate CRC
  uint8_t CRC = calculateCheckSum(_buffer);

  // Extract parameter length (big endian)
  uint16_t paramLength = _buffer[R200_ParamLengthMSBPos];
  paramLength <<= 8;
  paramLength += _buffer[R200_ParamLengthLSBPos];

  // Position of CRC in the buffer
  uint8_t CRCpos = 5 + paramLength;

  #ifdef DEBUG
    Serial.print("Calculated CRC: ");
    Serial.print(CRC, HEX);
    Serial.print(" Received CRC: ");
    Serial.println(_buffer[CRCpos], HEX);
  #endif

  return (CRC == _buffer[CRCpos]);
}

// Check if data is available from the RFID module
bool R200::dataAvailable(){
  return _serial->available() > 0;
}

// Dumps the most recently read UID to the serial output
void R200::dumpUIDToSerial(){
  Serial.print("0x");
  for (uint8_t i = 0; i < 12; i++){
    if(uid[i] < 0x10){
      Serial.print("0");
    }
    Serial.print(uid[i], HEX);
  }
}

// Dumps the receive buffer to the serial output (for debugging)
void R200::dumpReceiveBufferToSerial(){
  Serial.print("0x");
  for (uint8_t i = 0; i < RX_BUFFER_LENGTH; i++){
    if(_buffer[i] < 0x10){
      Serial.print("0");
    }
    Serial.print(_buffer[i], HEX);
  }
  Serial.println(". Done.");
}

// Parse data that has been placed in the receive buffer
bool R200::parseReceivedData() {
  switch(_buffer[R200_CommandPos]){
    case CMD_GetModuleInfo:
      // Parsing logic for GetModuleInfo if needed
      break;
    
    case CMD_SinglePollInstruction:
      for(uint8_t i = 8; i < 20; i++) {
        uid[i - 8] = _buffer[i];
      }
      // Alternatively: memcpy(uid, _buffer + 9, 12);
      break;
    
    case CMD_MultiplePollInstruction:
      for(uint8_t i = 8; i < 20; i++) {
        uid[i - 8] = _buffer[i];
      }
      // Alternatively: memcpy(uid, _buffer + 9, 12);
      break;
    
    case CMD_ExecutionFailure:
      // Handle ExecutionFailure if needed
      break;
    
    default:
      break;
  }
  return true; // Assuming successful parsing
}

// Flush the serial buffer by reading and discarding all available bytes
uint8_t R200::flush(){
  uint8_t bytesDiscarded = 0;
  while(_serial->available()){
    _serial->read();
    bytesDiscarded++;
  }
  return bytesDiscarded;
}

// Read incoming serial data sent by the reader
// Returns true if a complete frame of data is read within the allotted timeout
bool R200::receiveData(unsigned long timeOut){
  unsigned long startTime = millis();
  uint8_t bytesReceived = 0;
  
  // Clear the buffer
  memset(_buffer, 0, sizeof(_buffer));

  while ((millis() - startTime) < timeOut) {
    while (_serial->available()) {
      uint8_t b = _serial->read();
      if(bytesReceived >= RX_BUFFER_LENGTH - 1) {
        Serial.println("Error: Max Buffer Length Exceeded!");
        flush();
        return false;
      }
      else {
        _buffer[bytesReceived++] = b;
      }
      if (b == R200_FrameEnd) { 
        break; 
      }
    }
    // Optionally add a small delay to prevent overwhelming the loop
    delay(1);
  }

  if (bytesReceived > 1 && _buffer[0] == R200_FrameHeader && _buffer[bytesReceived - 1] == R200_FrameEnd) {
      return true;
  } else {
      return false;
  }
}

// Send a command to the RFID module: Get Module Info
void R200::dumpModuleInfo(){
  uint8_t commandFrame[8] = {0};
  commandFrame[0] = R200_FrameHeader;
  commandFrame[1] = FrameType_Command;
  commandFrame[2] = CMD_GetModuleInfo;
  commandFrame[3] = 0x00; // ParamLen MSB
  commandFrame[4] = 0x01; // ParamLen LSB
  commandFrame[5] = 0x00;  // Param
  // Calculate checksum: type + command + param length + param
  uint8_t checksum = FrameType_Command + CMD_GetModuleInfo + 0x00 + 0x01 + 0x00;
  commandFrame[6] = checksum & 0xFF;
  commandFrame[7] = R200_FrameEnd;
  _serial->write(commandFrame, 8);
}

// Send a command to the RFID module: Single Poll Instruction
void R200::poll(){
  uint8_t commandFrame[7] = {0};
  commandFrame[0] = R200_FrameHeader;
  commandFrame[1] = FrameType_Command;
  commandFrame[2] = CMD_SinglePollInstruction;
  commandFrame[3] = 0x00; // ParamLen MSB
  commandFrame[4] = 0x00; // ParamLen LSB
  commandFrame[5] = 0x22;  // Checksum: FrameType_Command + CMD_SinglePollInstruction + 0x00 + 0x00 = 0x00 + 0x22 + 0x00 + 0x00 = 0x22
  commandFrame[6] = R200_FrameEnd;
  _serial->write(commandFrame, 7);
}

// Enable or disable multiple polling mode
void R200::setMultiplePollingMode(bool enable){
  if(enable){
    uint8_t commandFrame[10] = {0};
    commandFrame[0] = R200_FrameHeader;
    commandFrame[1] = FrameType_Command; //(0x00)
    commandFrame[2] = CMD_MultiplePollInstruction; //0x27
    commandFrame[3] = 0x00; // ParamLen MSB
    commandFrame[4] = 0x03; // ParamLen LSB
    commandFrame[5] = 0x22;  // Param (Reserved? Always 0x22 for this command)
    commandFrame[6] = 0xFF;  // Param (Count of polls, MSB)
    commandFrame[7] = 0xFF;  // Param (Count of polls, LSB)
    // Calculate checksum: FrameType_Command + CMD_MultiplePollInstruction + 0x00 + 0x03 + 0x22 + 0xFF + 0xFF
    uint8_t checksum = FrameType_Command + CMD_MultiplePollInstruction + 0x00 + 0x03 + 0x22 + 0xFF + 0xFF;
    commandFrame[8] = checksum & 0xFF;
    commandFrame[9] = R200_FrameEnd;
    _serial->write(commandFrame, 10);
  }
  else {
    uint8_t commandFrame[7] = {0};
    commandFrame[0] = R200_FrameHeader;
    commandFrame[1] = FrameType_Command; //(0x00)
    commandFrame[2] = CMD_StopMultiplePoll; //0x28
    commandFrame[3] = 0x00; // ParamLen MSB
    commandFrame[4] = 0x00; // ParamLen LSB
    // Calculate checksum: FrameType_Command + CMD_StopMultiplePoll + 0x00 + 0x00 = 0x00 + 0x28 + 0x00 + 0x00 = 0x28
    commandFrame[5] = 0x28; 
    commandFrame[6] = R200_FrameEnd;
    _serial->write(commandFrame, 7);
  }
}

// Calculate the checksum for the given buffer
uint8_t R200::calculateCheckSum(uint8_t *buffer){
  // Extract how many parameters there are in the buffer
  uint16_t paramLength = buffer[R200_ParamLengthMSBPos];
  paramLength <<=8;
  paramLength += buffer[R200_ParamLengthLSBPos];
  
  // Checksum is calculated as the sum of all parameter bytes
  // added to four control bytes at the start (type, command, and the 2-byte parameter length)
  // Start from 1 to exclude frame header
  uint16_t check = 0;
  for(uint16_t i = 1; i < paramLength + 4 + 1; i++) { // i < 5 + paramLength
    check += buffer[i];
  }
  // Now only return LSB
  return (check & 0xFF);
}

// Convert an array of two bytes to a uint16_t (big endian)
uint16_t R200::arrayToUint16(uint8_t *array){
  uint16_t value = array[0];
  value <<=8;
  value += array[1];
  return value;
}
