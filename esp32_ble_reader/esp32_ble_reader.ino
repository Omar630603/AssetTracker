#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// === Wi-Fi Configuration ===
const char* ssid = "POCO X6 5G";
const char* password = "12345678";
const char* serverUrl = "https://f78a-2a09-bac1-34c0-40-00-3e7-9.ngrok-free.app/api"; // Base URL without /reader-config
const char* readerKey = "ESP32_READER_KEY";

// === Default Configuration ===
#define DEVICE_NAME "Asset_Reader_01"      // Reader name
String TARGET_DEVICE_NAME = "Asset_Tag_01"; // BLE tag name (can be updated from server)
float TX_POWER = -52;                       // Calibrated RSSI at 1 meter (adjust after field test)
float PATH_LOSS_EXPONENT = 2.5;             // Typical indoor; tune based on environment
float MAX_DISTANCE = 5.0;                   // Max allowed distance in meters
int SAMPLE_COUNT = 10;                      // Number of RSSI samples per batch
int SAMPLE_DELAY_MS = 100;                  // Delay between samples

// Kalman filter parameters (can be updated from server)
float kalman_P = 1.0;
float kalman_Q = 0.01;
float kalman_R = 1.0;
float kalman_initial = -50.0;

bool configFetched = false;

BLEScan* pBLEScan;
std::vector<String> targetDevices;

// === Kalman Filter Class ===
class KalmanFilter {
public:
  KalmanFilter(float processNoise, float measurementNoise, float estimatedError, float initialValue) {
    Q = processNoise;
    R = measurementNoise;
    P = estimatedError;
    X = initialValue;
  }

  void updateParameters(float processNoise, float measurementNoise, float estimatedError, float initialValue) {
    Q = processNoise;
    R = measurementNoise;
    P = estimatedError;
    X = initialValue;
  }

  float update(float measurement) {
    // Prediction update
    P = P + Q;

    // Measurement update
    K = P / (P + R);
    X = X + K * (measurement - X);
    P = (1 - K) * P;

    return X;
  }

private:
  float Q;  // Process noise covariance
  float R;  // Measurement noise covariance
  float P;  // Estimation error covariance
  float K;  // Kalman gain
  float X;  // Value
};

// === Initialize Kalman Filter ===
KalmanFilter rssiFilter(kalman_Q, kalman_R, kalman_P, kalman_initial);

// === Utility Function ===
float calculateDistance(float rssi) {
  if (rssi == 0) return -1.0;  // Avoid invalid calculations

  float ratio_db = TX_POWER - rssi;
  float distance = pow(10, ratio_db / (10 * PATH_LOSS_EXPONENT));

  // Apply piecewise correction for close range (<2 m)
  if (distance < 2.0) {
    distance *= 0.5;  // Example: halve distance for near-field correction
  }

  return distance;
}

bool fetchConfigFromServer() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Cannot fetch config.");
    return false;
  }

  HTTPClient http;
  String url = String(serverUrl) + "/reader-config?reader_name=" + DEVICE_NAME;
  
  Serial.print("Fetching config from: ");
  Serial.println(url);
  
  http.begin(url);
  http.addHeader("X-Reader-Key", readerKey);
  
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("Config received:");
    Serial.println(payload);
    
    // Use ArduinoJson to parse the response
    DynamicJsonDocument doc(2048); // Adjust size based on your expected config
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
      Serial.print("JSON parsing failed: ");
      Serial.println(error.c_str());
      http.end();
      return false;
    }
    
    // Clear existing targets and add new ones
    targetDevices.clear();
    JsonArray targets = doc["targets"];
    for (JsonVariant target : targets) {
      targetDevices.push_back(target.as<String>());
    }
    
    // If targets array is empty, use the default target
    if (targetDevices.empty()) {
      targetDevices.push_back(TARGET_DEVICE_NAME);
    }
    
    // Update configuration parameters if present
    JsonObject config = doc["config"];
    if (!config.isNull()) {
      // Update Kalman filter parameters
      if (!config["kalman"].isNull()) {
        kalman_P = config["kalman"]["P"] | kalman_P;
        kalman_Q = config["kalman"]["Q"] | kalman_Q;
        kalman_R = config["kalman"]["R"] | kalman_R;
        kalman_initial = config["kalman"]["initial"] | kalman_initial;
        
        // Update the Kalman filter with new parameters
        rssiFilter.updateParameters(kalman_Q, kalman_R, kalman_P, kalman_initial);
      }
      
      TX_POWER = config["txPower"] | TX_POWER;
      MAX_DISTANCE = config["maxDistance"] | MAX_DISTANCE;
      SAMPLE_COUNT = config["sampleCount"] | SAMPLE_COUNT;
      SAMPLE_DELAY_MS = config["sampleDelayMs"] | SAMPLE_DELAY_MS;
      PATH_LOSS_EXPONENT = config["pathLossExponent"] | PATH_LOSS_EXPONENT;
    }
    
    http.end();
    return true;
  } else {
    Serial.print("HTTP request failed with error code: ");
    Serial.println(httpCode);
    String payload = http.getString();
    Serial.println(payload);
    http.end();
    return false;
  }
}

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  
  // Wait for connection with a timeout
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.print("Connected to WiFi. IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("");
    Serial.println("Failed to connect to WiFi. Will use default configuration.");
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("Starting Asset Tracking System...");
  
  // Initialize WiFi and fetch configuration
  connectToWiFi();
  
  if (WiFi.status() == WL_CONNECTED) {
    configFetched = fetchConfigFromServer();
    if (configFetched) {
      Serial.println("Configuration successfully applied from server.");
    } else {
      Serial.println("Using default configuration.");
    }
  }
  
  // Add the default target if no targets were fetched
  if (targetDevices.empty()) {
    targetDevices.push_back(TARGET_DEVICE_NAME);
  }
  
  // Initialize BLE
  BLEDevice::init(DEVICE_NAME);
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
  pBLEScan->setActiveScan(true);
  
  Serial.println("BLE scanner initialized.");
  
  // Print current configuration
  Serial.println("Current configuration:");
  Serial.print("TX Power: ");
  Serial.println(TX_POWER);
  Serial.print("Path Loss Exponent: ");
  Serial.println(PATH_LOSS_EXPONENT);
  Serial.print("Max Distance: ");
  Serial.println(MAX_DISTANCE);
  Serial.print("Sample Count: ");
  Serial.println(SAMPLE_COUNT);
  Serial.print("Target Devices: ");
  for (const String& target : targetDevices) {
    Serial.println(target);
  }
}

void loop() {
  // Check if we should try to update config (every 5 minutes)
  static unsigned long lastConfigCheck = 0;
  if (millis() - lastConfigCheck > 300000) { // 5 minutes in milliseconds
    if (WiFi.status() == WL_CONNECTED) {
      fetchConfigFromServer();
    }
    lastConfigCheck = millis();
  }
  
  // Process all target devices
  for (const String& currentTarget : targetDevices) {
    Serial.print("Scanning for target: ");
    Serial.println(currentTarget);
    
    int sampleCounter = 0;
    float rssiSum = 0;
    
    unsigned long startTime = millis();
    
    // Collect RSSI samples
    while (sampleCounter < SAMPLE_COUNT && (millis() - startTime < 30000)) { // 30 second timeout
      BLEScanResults* foundDevices = pBLEScan->start(1, false);  // 1 second scan
      
      for (int i = 0; i < foundDevices->getCount(); i++) {
        BLEAdvertisedDevice device = foundDevices->getDevice(i);
        
        if (device.getName() == currentTarget) {
          int rssi = device.getRSSI();
          rssiSum += rssi;
          sampleCounter++;
          
          Serial.print("Sample ");
          Serial.print(sampleCounter);
          Serial.print(": Raw RSSI = ");
          Serial.println(rssi);
          
          if (sampleCounter >= SAMPLE_COUNT) {
            break;
          }
        }
      }
      
      pBLEScan->clearResults();
      delay(SAMPLE_DELAY_MS);  // Configurable delay between scans
    }
    
    // Process results if we collected enough samples
    if (sampleCounter > 0) {
      float rssiAvg = rssiSum / sampleCounter;
      float filteredRssi = rssiFilter.update(rssiAvg);
      
      float rawDistance = calculateDistance(rssiAvg);
      float filteredDistance = calculateDistance(filteredRssi);
      
      Serial.print("Device: ");
      Serial.print(currentTarget);
      Serial.print(" | Avg RSSI = ");
      Serial.print(rssiAvg);
      Serial.print(" dBm | Filtered RSSI = ");
      Serial.print(filteredRssi);
      Serial.print(" dBm | Raw Distance = ");
      Serial.print(rawDistance, 2);
      Serial.print(" m | Filtered Distance = ");
      Serial.print(filteredDistance, 2);
      Serial.println(" m");
      
      // Check distance threshold
      if (filteredDistance > MAX_DISTANCE) {
        Serial.print("⚠ ALERT: Asset ");
        Serial.print(currentTarget);
        Serial.println(" has likely left the room (distance > threshold)");
        // TODO: Send alert signal to server here
      } else {
        Serial.print("✅ Asset ");
        Serial.print(currentTarget);
        Serial.println(" is within allowed range.");
        // TODO: Send heartbeat/status signal to server here
      }
    } else {
      Serial.print("❌ Device ");
      Serial.print(currentTarget);
      Serial.println(" not found during scan period.");
    }
  }
  
  delay(500);  // Half a second pause before next round
}