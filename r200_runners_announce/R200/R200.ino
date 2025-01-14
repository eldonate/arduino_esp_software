/*
  由四川睿频科技有限公司编写
  复制及使用请保留版权所属

  Adapted for ESP32 by <Your Name/Company>.
*/

#include <WiFi.h>
#include <HTTPClient.h>

// WiFi credentials
const char* ssid     = "eldonate";
const char* password = "adriano90.";

// Server URL (replace with your actual endpoint)
const char* serverName = "http://192.168.1.248/rfid_submit.php";

// Define RFID UART pins
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

// Buffer to store EPC bytes
#define EPC_LENGTH 12  // Number of EPC bytes based on your example
unsigned char epcBuffer[EPC_LENGTH];
unsigned int epcIndex = 0;

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
  
  // Connect to WiFi
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  // Wait until connected
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
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
      epcIndex  = 0;  // Reset EPC buffer index
    }
    else if (codeState == 1) {
      dataAdd++;
      // Start collecting EPC bytes (assuming EPC starts at dataAdd = 9)
      if ((dataAdd >= 9) && (dataAdd <= (8 + EPC_LENGTH))) {
        if (epcIndex < EPC_LENGTH) {
          epcBuffer[epcIndex++] = incomedate;
        }
      }
      // End of EPC frame
      else if (dataAdd == (8 + EPC_LENGTH + 1)) {  // Adjust based on your frame structure
        // Convert EPC buffer to hexadecimal string
        String epcString = "";
        for (int i = 0; i < epcIndex; i++) {
          if (epcBuffer[i] < 0x10) {
            epcString += "0";  // Leading zero for single hex digit
          }
          epcString += String(epcBuffer[i], HEX);
        }
        epcString.toUpperCase();  // Convert to uppercase for consistency

        // Print only the EPC string
        Serial.println(epcString);

        // Send EPC to PHP server
        sendRFID(epcString);

        // Reset parsing state
        dataAdd    = 0;
        parState   = 0;
        codeState  = 0;
        epcIndex   = 0;
      }
      // If dataAdd exceeds expected frame length, reset
      else if (dataAdd > (8 + EPC_LENGTH + 1)) {
        // Reset parsing state
        dataAdd    = 0;
        parState   = 0;
        codeState  = 0;
        epcIndex   = 0;
      }
    }
    // Reset parsing state if unexpected data is received
    else {
      dataAdd    = 0;
      parState   = 0;
      codeState  = 0;
      epcIndex   = 0;
    }
  }
  
  // Optional: Small delay to prevent excessive CPU usage
  delay(1);
}

// Function to send RFID data to PHP server
void sendRFID(String epc) {
  if (WiFi.status() == WL_CONNECTED) {  // Check WiFi connection status
    HTTPClient http;

    // Prepare URL with query parameter
    String url = String(serverName) + "?tag=" + epc;
    
    http.begin(url.c_str());  // Specify the URL
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");  // Specify content type

    // Send HTTP GET request
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      // Optionally, read the response
      String response = http.getString();
      Serial.println(response);
    }
    else {
      Serial.print("Error on sending GET: ");
      Serial.println(httpResponseCode);
    }

    http.end();  // Free resources
  }
  else {
    Serial.println("WiFi Disconnected");
  }
}
