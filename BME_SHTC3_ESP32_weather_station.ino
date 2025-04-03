#include "SparkFun_SHTC3.h"
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <WebServer.h>
#include <WiFi.h>
#include "esp_sleep.h"
#include <HTTPClient.h>

// Network credentials
const char* ssid = "WifiNetwork92";
const char* password = "*Password_here*";

// Sensors and Web Server
SHTC3 shtc3;
Adafruit_BME280 bme;
WebServer server(80);
TwoWire I2CBME = TwoWire(1);

const float elevation = 59.0; // Elevation for sea-level pressure calculation

// Constants for sleep and active duration (in microseconds)
const uint64_t SLEEP_DURATION = 15 * 60 * 1000000ULL; // 15 minutes
const unsigned long ACTIVE_DURATION = 20000;           // 20 seconds (in milliseconds)

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  // Wait for Wi-Fi connection
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\nConnected to Wi-Fi\nIP address: %s\n", WiFi.localIP().toString().c_str());

  // Initialize sensors
  Wire.begin(10, 9);
  I2CBME.begin(14, 13);

  if (shtc3.begin() != SHTC3_Status_Nominal) {
    Serial.println("SHTC3 initialization failed!");
    enterDeepSleep();  // Sleep if initialization fails
  } else {
    Serial.println("SHTC3 initialization successful.");
  }

  if (!bme.begin(0x76, &I2CBME)) {
    Serial.println("BME280 not found!");
    enterDeepSleep();  // If the sensor fails, go to deep sleep immediately
  } else {
    Serial.println("BME280 initialization successful.");
  }

  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();

  // Log wakeup reason
  printWakeupReason();
}

void loop() {
  unsigned long startMillis = millis();

  // Serve sensor data for the defined active duration (20 seconds)
  while (millis() - startMillis < ACTIVE_DURATION) {
    server.handleClient(); // Handle web requests
    shtc3.update();      // Update SHTC3 sensor data
    handleData();
    delay(1000);           // Sample delay for stable readings
  }

  // Put sensors to sleep and enter deep sleep
  Serial.println("Putting sensors to sleep...");
  shtc3.sleep();  // Put SHTC3 to sleep
  setBME280ForcedMode();  // Put BME280 in forced mode

  enterDeepSleep();  // ESP32 enters deep sleep
}

void enterDeepSleep() {
  Serial.println("Entering deep sleep for 15 minutes...");
  esp_sleep_enable_timer_wakeup(SLEEP_DURATION);
  esp_deep_sleep_start();
}

void handleRoot() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/html", 
    "<html><body><h1>BME280 Sensor Data</h1><p>Use /data for JSON.</p></body></html>");
}

void handleData() {
  Serial.println("Handling /data request...");

  float shtc3Temp = shtc3.toDegF();
  float humidity = shtc3.toPercent();

  // Trigger a single forced mode measurement
  bme.takeForcedMeasurement();  
  float pressure = bme.readPressure() / 100.0F;

  // Create the JSON payload to send to the Flask server
  String payload = "{\"temperature\":" + String(shtc3Temp) + 
                    ",\"humidity\":" + String(humidity) + 
                    ",\"pressure\":" + String(pressure) + "}";
  Serial.print("Sending data: ");
  Serial.println(payload);

  // Send the POST request to the Flask API
  HTTPClient http;
  http.begin("http://192.168.0.18:5000/api/data");  // Replace with your Flask server's IP
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST(payload);  // Send the POST request

  if (httpCode > 0) {
    Serial.printf("POST Request successful. HTTP Code: %d\n", httpCode);
  } else {
    Serial.printf("POST Request failed. Error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();  // Close the HTTP connection
}

void printWakeupReason() {
  esp_sleep_wakeup_cause_t wakeupReason = esp_sleep_get_wakeup_cause();
  switch (wakeupReason) {
    case ESP_SLEEP_WAKEUP_TIMER:
      Serial.println("Wakeup caused by timer");
      break;
    default:
      Serial.println("Wakeup not caused by timer");
      break;
  }

  // Wake up sensors after deep sleep
  Serial.println("Waking up sensors...");
  shtc3.wake();  // Wake SHTC3
  bme.begin(0x76, &I2CBME);  // Reinitialize BME280
}

float celsiusToFahrenheit(float tempC) {
  return (tempC * 9.0 / 5.0) + 32.0;
}

float calculateSeaLevelPressure(float pressure, float elevation) {
  return pressure / pow(1.0 - (elevation / 44330.0), 5.255);
}

// Function to set BME280 in forced mode to minimize power consumption
void setBME280ForcedMode() {
  bme.setSampling(Adafruit_BME280::MODE_FORCED,
                  Adafruit_BME280::SAMPLING_X1,  // Temperature sampling
                  Adafruit_BME280::SAMPLING_X1,  // Pressure sampling
                  Adafruit_BME280::SAMPLING_X1,  // Humidity sampling
                  Adafruit_BME280::FILTER_OFF);  // No filtering
}
