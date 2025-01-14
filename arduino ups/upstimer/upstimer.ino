#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Initialize the display with I2C pins (SCL = 22, SDA = 21)
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/22, /* data=*/21, U8X8_PIN_NONE);

// Wi-Fi credentials
const char* ssid = "eldonate";
const char* password = "adriano90.";

// Server details
const char* serverName = "http://192.168.1.248/get_last_entry.php"; // Ensure this URL is correct

// Function to fetch data from server
String fetchLastEntry() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverName);

    // Send GET request
    int httpCode = http.GET();

    String payload = "";
    if (httpCode > 0) { // Check for the returning code
      if (httpCode == HTTP_CODE_OK) {
        payload = http.getString();
        Serial.println("Response: " + payload);
      }
    } else {
      Serial.println("Error on HTTP request");
    }

    http.end(); // Free resources
    return payload;
  } else {
    Serial.println("WiFi Disconnected");
    return "";
  }
}

// Function to truncate strings to prevent overflow
String truncateString(String str, int maxLength) {
  if (str.length() > maxLength) {
    return str.substring(0, maxLength - 3) + "...";
  } else {
    return str;
  }
}

void setup() {
  // Initialize Serial and Display
  Serial.begin(115200);
  u8g2.begin();

  // Initialize Wi-Fi
  WiFi.begin(ssid, password);

  // Display "Connecting to Wi-Fi..."
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr); // Neoclassical font with Greek support
  u8g2.drawUTF8(0, 20, "Connecting to Wi-Fi...");
  u8g2.sendBuffer();

  // Wait for Wi-Fi connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Display "Wi-Fi Connected!"
  u8g2.clearBuffer();
  u8g2.drawUTF8(0, 20, "Wi-Fi Connected!");
  u8g2.sendBuffer();

  Serial.println("\nConnected to Wi-Fi!");
}

void loop() {
  // Clear the buffer
  u8g2.clearBuffer();

  // Check Wi-Fi status and display it
  if (WiFi.status() == WL_CONNECTED) {
    u8g2.drawUTF8(0, 10, "Wi-Fi Connected");
  } else {
    u8g2.drawUTF8(0, 10, "Wi-Fi Disconnected");
  }

  // Fetch data from server
  String response = fetchLastEntry();
  String firstName = "N/A";
  String lastName = "N/A";
  String bib = "N/A";

  if (response.length() > 0) {
    // Parse JSON
    StaticJsonDocument<200> doc; // Adjust size based on expected JSON

    DeserializationError error = deserializeJson(doc, response);

    if (!error) {
      if (doc.containsKey("last_name") && doc.containsKey("first_name") && doc.containsKey("bib")) {
        lastName = doc["last_name"].as<String>();
        firstName = doc["first_name"].as<String>();
        bib = doc["bib"].as<String>();

        // Truncate strings if necessary
        firstName = truncateString(firstName, 20);
        lastName = truncateString(lastName, 20);
        bib = truncateString(bib, 10);
      } else if (doc.containsKey("error")) {
        firstName = "Error: " + String(doc["error"].as<String>());
        firstName = truncateString(firstName, 20);
      }
    } else {
      Serial.println("JSON Parsing Failed");
    }
  }

  // Display the fetched first_name, last_name, and bib on separate lines without labels
  u8g2.setFont(u8g2_font_ncenB08_tr); // Using a readable font that supports Greek
  u8g2.drawUTF8(0, 30, firstName.c_str());
  u8g2.drawUTF8(0, 45, lastName.c_str());
  u8g2.drawUTF8(0, 60, bib.c_str());

  // Send buffer to the display
  u8g2.sendBuffer();

  // Print information to Serial Monitor
  Serial.println("First Name: " + firstName);
  Serial.println("Last Name: " + lastName);
  Serial.println("Bib: " + bib);

  // Wait for 10 seconds before next operation
  delay(1000);
}
