#ifndef R200_h
#define R200_h

// Uncomment the following line to enable debug information
#define DEBUG

#include <stdint.h>
#include <Arduino.h>
#include <Stream.h>  // Include Stream for general serial communication

#define RX_BUFFER_LENGTH 64

class R200 {

  private:
    Stream *_serial;  // Changed from HardwareSerial* to Stream*
    uint8_t _buffer[RX_BUFFER_LENGTH] = {0};
    uint8_t calculateCheckSum(uint8_t *buffer);
    uint16_t arrayToUint16(uint8_t *array);
    bool parseReceivedData();
    bool dataIsValid();
    bool receiveData(unsigned long timeOut = 500);
    void dumpReceiveBufferToSerial();
    uint8_t flush();

    const uint8_t blankUid[12] = {0};

  public:
    R200();

    uint8_t uid[12] = {0};

    bool begin(Stream *serial);  // Removed baud parameter
    void loop();
    void poll();
    void setMultiplePollingMode(bool enable=true);
    void dumpModuleInfo();
    bool dataAvailable();

    void dumpUIDToSerial();

    // Enums remain unchanged
    enum R200_FrameStructure : byte {
      R200_HeaderPos = 0x00,
      R200_TypePos = 0x01,
      R200_CommandPos = 0x02,
      R200_ParamLengthMSBPos = 0x03,
      R200_ParamLengthLSBPos = 0x04,
      R200_ParamPos = 0x05,
    };

    enum R200_FrameControl : byte {
      R200_FrameHeader = 0xAA,
      R200_FrameEnd = 0xDD,
    };

    enum R200_FrameType : byte {
      FrameType_Command = 0x00,
      FrameType_Response = 0x01,
      FrameType_Notification = 0x02,
    };

    enum R200_Command : byte {
      CMD_GetModuleInfo = 0x03,
      CMD_SinglePollInstruction = 0x22,
      CMD_MultiplePollInstruction = 0x27,
      CMD_StopMultiplePoll = 0x28,
      CMD_SetSelectParameter = 0x0C,
      CMD_GetSelectParameter = 0x0B,
      CMD_SetSendSelectInstruction = 0x12,
      CMD_ReadLabel = 0x39,
      CMD_WriteLabel = 0x49,
      CMD_LockLabel = 0x82,
      CMD_KillTag = 0x65,
      CMD_GetQueryParameters = 0x0D,
      CMD_SetQueryParameters= 0x0E,
      CMD_SetWorkArea = 0x07,
      CMD_SetWorkingChannel = 0xAB,
      CMD_GetWorkingChannel = 0xAA,
      CMD_SetAutoFrequencyHopping = 0xAD,
      CMD_AcquireTransmitPower = 0xB7,
      CMD_SetTransmitPower = 0xB6,
      CMD_SetTransmitContinuousCarrier = 0xB0,
      CMD_GetReceiverDemodulatorParameters = 0xF1,
      CMD_SetReceiverDemodulatorParameters = 0xF0,
      CMD_TestRFInputBlockingSignal = 0xF2,
      CMD_TestChannelRSSI = 0xF3,
      CMD_ControlIOPort = 0x1A,
      CMD_ModuleSleep = 0x17,
      CMD_SetModuleIdleSleepTime = 0x1D,
      CMD_ExecutionFailure = 0xFF,
    };

    enum R200_ErrorCode : byte {
      ERR_CommandError = 0x17,
      ERR_FHSSFail = 0x20,
      ERR_InventoryFail = 0x15,
      ERR_AccessFail = 0x16,
      ERR_ReadFail = 0x09,
      ERR_WriteFail = 0x10,
      ERR_LockFail = 0x13,
      ERR_KillFail = 0x12,
    };
};
#endif
