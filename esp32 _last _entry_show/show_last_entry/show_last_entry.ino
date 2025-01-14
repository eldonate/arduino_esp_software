// WiFi connection for ESP32
#include <WiFi.h>

// WiFi credentials
const char* ssid = "eldonate";
const char* password = "adriano90.";

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Keep-alive loop
  delay(1000);
}
