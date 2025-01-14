/*
  由四川睿频科技有限公司编写
  复制及使用请保留版权所属

  Adapted for ESP32 by <Your Name/Company>.
*/

#define RFID_RX_PIN 16  // GPIO16 (RX2)
#define RFID_TX_PIN 17  // GPIO17 (TX2)
#define LED_PIN     2   // Onboard LED (common for many ESP32 boards)

// Multi-read command
unsigned char ReadMulti[10] = {0xAA, 0x00, 0x27, 0x00, 0x03, 0x22, 0xFF, 0xFF, 0x4A, 0xDD};

// Timing variables
unsigned long timeSec   = 0;
unsigned long timemin   = 0;

// Parsing variables
unsigned int dataAdd    = 0;
unsigned int incomedate = 0;
unsigned int parState   = 0;
unsigned int codeState  = 0;

// Initialize Serial2 for RFID
HardwareSerial RFIDSerial(2);

void setup() {
  // Initialize Serial for debugging (connected to USB)
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for Serial to be ready
  }
  
  // Initialize Serial2 for RFID communication
  RFIDSerial.begin(115200, SERIAL_8N1, RFID_RX_PIN, RFID_TX_PIN);
  
  // Initialize LED pin
  pinMode(LED_PIN, OUTPUT);
  
  // Send initial messages via Serial (to monitor)
  Serial.println("Hello world. (ESP32 Version)");
  
  // Send initial ReadMulti command via Serial2 (to RFID)
  RFIDSerial.write(ReadMulti, sizeof(ReadMulti));
}

void loop() {
  // Timing logic to send ReadMulti periodically
  timeSec++;
  if (timeSec >= 50000) {  // Adjust this value as needed
    timemin++;
    timeSec = 0;
    if (timemin >= 20) {  // Adjust this value as needed
      timemin = 0;
      // Indicate activity with LED
      digitalWrite(LED_PIN, HIGH);
      
      // Send ReadMulti command via Serial2 to RFID
      RFIDSerial.write(ReadMulti, sizeof(ReadMulti));
      
      digitalWrite(LED_PIN, LOW);
    }
  }

  // Check if data is available from RFID (Serial2)
  while (RFIDSerial.available() > 0) {
    incomedate = RFIDSerial.read();

    // Parsing logic to interpret RFID data
    if ((incomedate == 0x02) && (parState == 0)) {
      parState = 1;
    }
    else if ((parState == 1) && (incomedate == 0x22) && (codeState == 0)) {
      codeState = 1;
      dataAdd   = 3;  // Start counting from the data position
    }
    else if (codeState == 1) {
      dataAdd++;
      // Get RSSI (6th byte)
      if (dataAdd == 6) {
        Serial.print("RSSI: ");
        Serial.println(incomedate, HEX);
      }
      // Get PC code (7th and 8th bytes)
      else if ((dataAdd == 7) || (dataAdd == 8)) {
        if (dataAdd == 7) {
          Serial.print("PC: ");
          Serial.print(incomedate, HEX);
        }
        else {
          Serial.println(incomedate, HEX);
        }
      }
      // Get EPC (9th to 20th bytes)
      else if ((dataAdd >= 9) && (dataAdd <= 20)) {
        if (dataAdd == 9) {
          Serial.print("EPC: ");
        }
        Serial.print(incomedate, HEX);
      }
      // End of frame
      else if (dataAdd >= 21) {
        Serial.println();  // Newline after EPC
        // Reset parsing state
        dataAdd    = 0;
        parState   = 0;
        codeState  = 0;
      }
    }
    // Reset parsing state if unexpected data is received
    else {
      dataAdd    = 0;
      parState   = 0;
      codeState  = 0;
    }
  }
  
  // Optional: Small delay to prevent excessive CPU usage
  delay(1);
}
