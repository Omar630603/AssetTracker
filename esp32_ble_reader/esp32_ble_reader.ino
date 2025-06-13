#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// === Wi-Fi Configuration ===
const char* ssid = "POCO X6 5G";
const char* password = "12345678";
const char* serverUrl = "https://2c55-2404-c0-5a30-00-10bd-d8af.ngrok-free.app/api"; 
const char* readerKey = "ESP32_READER_KEY";

// === Default Configuration ===
#define DEVICE_NAME "Asset_Reader_01"
String TARGET_DEVICE_NAME = "Asset_Tag_01";
float TX_POWER = -52;
float PATH_LOSS_EXPONENT = 2.5;
float MAX_DISTANCE = 5.0;
int SAMPLE_COUNT = 10;
int SAMPLE_DELAY_MS = 100;

// Kalman filter parameters
float kalman_P = 1.0;
float kalman_Q = 0.01;
float kalman_R = 1.0;
float kalman_initial = -50.0;

// === Smart Polling Variables ===
bool configFetched = false;
unsigned long lastConfigVersion = 0;
unsigned long currentConfigVersion = 0;
bool forceConfigUpdate = false;

// === Alert and Status Tracking ===
struct DeviceStatus {
  String name;
  bool isPresent;
  int consecutiveAlerts;
  int consecutiveStatus;
  unsigned long lastAlertSent;
  unsigned long lastStatusSent;
  float lastDistance;
};

BLEScan* pBLEScan;
std::vector<DeviceStatus> deviceStatuses;

const int ALERT_CONFIRMATION_COUNT = 3; // Send alert after 3 consecutive detections
const int STATUS_CONFIRMATION_COUNT = 5; // Send status after 5 consecutive detections
const unsigned long MIN_ALERT_INTERVAL = 60000; // 1 minute between same alerts
const unsigned long MIN_STATUS_INTERVAL = 300000; // 5 minutes between status updates

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
    Serial.println("[KALMAN] Parameters updated - Q:" + String(Q) + " R:" + String(R) + " P:" + String(P) + " Initial:" + String(X));
  }

  float update(float measurement) {
    P = P + Q;
    K = P / (P + R);
    X = X + K * (measurement - X);
    P = (1 - K) * P;
    return X;
  }

private:
  float Q, R, P, K, X;
};

KalmanFilter rssiFilter(kalman_Q, kalman_R, kalman_P, kalman_initial);

float calculateDistance(float rssi) {
  if (rssi == 0) return -1.0;
  float ratio_db = TX_POWER - rssi;
  float distance = pow(10, ratio_db / (10 * PATH_LOSS_EXPONENT));
  if (distance < 2.0) {
    distance *= 0.5;
  }
  return distance;
}

bool sendAlert(const String& deviceName, float distance, const String& alertType) {
  Serial.println("[ALERT] Attempting to send alert for " + deviceName + " type:" + alertType + " distance:" + String(distance));
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[ALERT] Failed - WiFi not connected");
    Serial.println("[ALERT] WiFi status: " + String(WiFi.status()));
    return false;
  }

  HTTPClient http;
  String url = String(serverUrl) + "/reader-alert";
  
  Serial.println("[ALERT] URL: " + url);
  Serial.println("[ALERT] Using Reader Key: " + String(readerKey));
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Reader-Key", readerKey);
  http.setTimeout(10000); // 10 second timeout
  
  DynamicJsonDocument doc(512);
  doc["reader_name"] = DEVICE_NAME;
  doc["device_name"] = deviceName;
  doc["alert_type"] = alertType;
  doc["distance"] = distance;
  doc["timestamp"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial.println("[ALERT] Sending JSON: " + jsonString);
  Serial.println("[ALERT] Content-Length: " + String(jsonString.length()));
  
  int httpCode = http.POST(jsonString);
  Serial.println("[ALERT] HTTP code: " + String(httpCode));
  
  if (httpCode > 0) {
    String response = http.getString();
    Serial.println("[ALERT] Response length: " + String(response.length()));
    Serial.println("[ALERT] Response: " + response);
    
    bool success = (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED);
    if (success) {
      Serial.println("[ALERT] Successfully sent");
    } else {
      Serial.println("[ALERT] Failed with valid HTTP response - code: " + String(httpCode));
    }
    
    http.end();
    return success;
  } else {
    Serial.println("[ALERT] HTTP Error Details:");
    Serial.println("[ALERT] Error code: " + String(httpCode));
    
    switch(httpCode) {
      case HTTPC_ERROR_CONNECTION_REFUSED:
        Serial.println("[ALERT] Connection refused by server");
        break;
      case HTTPC_ERROR_SEND_HEADER_FAILED:
        Serial.println("[ALERT] Send header failed");
        break;
      case HTTPC_ERROR_SEND_PAYLOAD_FAILED:
        Serial.println("[ALERT] Send payload failed");
        break;
      case HTTPC_ERROR_NOT_CONNECTED:
        Serial.println("[ALERT] Not connected to server");
        break;
      case HTTPC_ERROR_CONNECTION_LOST:
        Serial.println("[ALERT] Connection lost");
        break;
      case HTTPC_ERROR_NO_STREAM:
        Serial.println("[ALERT] No stream available");
        break;
      case HTTPC_ERROR_NO_HTTP_SERVER:
        Serial.println("[ALERT] No HTTP server");
        break;
      case HTTPC_ERROR_TOO_LESS_RAM:
        Serial.println("[ALERT] Too less RAM");
        break;
      case HTTPC_ERROR_ENCODING:
        Serial.println("[ALERT] Encoding error");
        break;
      case HTTPC_ERROR_STREAM_WRITE:
        Serial.println("[ALERT] Stream write error");
        break;
      case HTTPC_ERROR_READ_TIMEOUT:
        Serial.println("[ALERT] Read timeout");
        break;
      default:
        Serial.println("[ALERT] Unknown HTTP error");
        break;
    }
    
    String errorString = http.errorToString(httpCode);
    Serial.println("[ALERT] Error string: " + errorString);
    
    http.end();
    return false;
  }
}

bool sendHeartbeat(const String& deviceName, float distance) {
  Serial.println("[HEARTBEAT] Attempting to send for " + deviceName + " distance:" + String(distance));
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[HEARTBEAT] Failed - WiFi not connected");
    Serial.println("[HEARTBEAT] WiFi status: " + String(WiFi.status()));
    return false;
  }

  HTTPClient http;
  String url = String(serverUrl) + "/reader-heartbeat";
  
  Serial.println("[HEARTBEAT] URL: " + url);
  Serial.println("[HEARTBEAT] Using Reader Key: " + String(readerKey));
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Reader-Key", readerKey);
  http.setTimeout(10000); // 10 second timeout
  
  DynamicJsonDocument doc(512);
  doc["reader_name"] = DEVICE_NAME;
  doc["device_name"] = deviceName;
  doc["distance"] = distance;
  doc["status"] = "present";
  doc["timestamp"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial.println("[HEARTBEAT] Sending JSON: " + jsonString);
  Serial.println("[HEARTBEAT] Content-Length: " + String(jsonString.length()));
  
  int httpCode = http.POST(jsonString);
  Serial.println("[HEARTBEAT] HTTP code: " + String(httpCode));
  
  if (httpCode > 0) {
    String response = http.getString();
    Serial.println("[HEARTBEAT] Response length: " + String(response.length()));
    Serial.println("[HEARTBEAT] Response: " + response);
    
    bool success = (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED);
    if (success) {
      Serial.println("[HEARTBEAT] Successfully sent");
    } else {
      Serial.println("[HEARTBEAT] Failed with valid HTTP response - code: " + String(httpCode));
    }
    
    http.end();
    return success;
  } else {
    Serial.println("[HEARTBEAT] HTTP Error Details:");
    Serial.println("[HEARTBEAT] Error code: " + String(httpCode));
    
    switch(httpCode) {
      case HTTPC_ERROR_CONNECTION_REFUSED:
        Serial.println("[HEARTBEAT] Connection refused by server");
        break;
      case HTTPC_ERROR_SEND_HEADER_FAILED:
        Serial.println("[HEARTBEAT] Send header failed");
        break;
      case HTTPC_ERROR_SEND_PAYLOAD_FAILED:
        Serial.println("[HEARTBEAT] Send payload failed");
        break;
      case HTTPC_ERROR_NOT_CONNECTED:
        Serial.println("[HEARTBEAT] Not connected to server");
        break;
      case HTTPC_ERROR_CONNECTION_LOST:
        Serial.println("[HEARTBEAT] Connection lost");
        break;
      case HTTPC_ERROR_NO_STREAM:
        Serial.println("[HEARTBEAT] No stream available");
        break;
      case HTTPC_ERROR_NO_HTTP_SERVER:
        Serial.println("[HEARTBEAT] No HTTP server");
        break;
      case HTTPC_ERROR_TOO_LESS_RAM:
        Serial.println("[HEARTBEAT] Too less RAM");
        break;
      case HTTPC_ERROR_ENCODING:
        Serial.println("[HEARTBEAT] Encoding error");
        break;
      case HTTPC_ERROR_STREAM_WRITE:
        Serial.println("[HEARTBEAT] Stream write error");
        break;
      case HTTPC_ERROR_READ_TIMEOUT:
        Serial.println("[HEARTBEAT] Read timeout");
        break;
      default:
        Serial.println("[HEARTBEAT] Unknown HTTP error");
        break;
    }
    
    String errorString = http.errorToString(httpCode);
    Serial.println("[HEARTBEAT] Error string: " + errorString);
    
    http.end();
    return false;
  }
}

bool checkConfigVersion() {
  Serial.println("[CONFIG] Checking version - Current:" + String(lastConfigVersion));
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[CONFIG] Version check failed - WiFi not connected");
    Serial.println("[CONFIG] WiFi status: " + String(WiFi.status()));
    return false;
  }

  HTTPClient http;
  String url = String(serverUrl) + "/reader-config?reader_name=" + DEVICE_NAME + "&version_check=true";
  
  Serial.println("[CONFIG] Version check URL: " + url);
  Serial.println("[CONFIG] Using Reader Key: " + String(readerKey));
  
  http.begin(url);
  http.addHeader("User-Agent", "ESP32HTTPClient/1.0");
  http.addHeader("ngrok-skip-browser-warning", "true");
  http.addHeader("X-Reader-Key", readerKey);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(10000); // 10 second timeout
  
  Serial.println("[CONFIG] Sending version check request...");
  int httpCode = http.GET();
  Serial.println("[CONFIG] Version check HTTP code: " + String(httpCode));
  
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("[CONFIG] Version response length: " + String(payload.length()));
    Serial.println("[CONFIG] Version response: " + payload);
    
    if (httpCode == HTTP_CODE_OK) {
      DynamicJsonDocument doc(512);
      DeserializationError error = deserializeJson(doc, payload);
      
      if (!error) {
        currentConfigVersion = doc["version"] | 0;
        Serial.println("[CONFIG] Server version:" + String(currentConfigVersion) + " Local version:" + String(lastConfigVersion));
        
        if (currentConfigVersion != lastConfigVersion) {
          Serial.println("[CONFIG] Version mismatch - Update needed");
          http.end();
          return true;
        } else {
          Serial.println("[CONFIG] Version up to date");
        }
      } else {
        Serial.println("[CONFIG] JSON parse error: " + String(error.c_str()));
      }
    }
  } else {
    Serial.println("[CONFIG] HTTP Error Details:");
    Serial.println("[CONFIG] Error code: " + String(httpCode));
    
    switch(httpCode) {
      case HTTPC_ERROR_CONNECTION_REFUSED:
        Serial.println("[CONFIG] Connection refused by server");
        break;
      case HTTPC_ERROR_SEND_HEADER_FAILED:
        Serial.println("[CONFIG] Send header failed");
        break;
      case HTTPC_ERROR_SEND_PAYLOAD_FAILED:
        Serial.println("[CONFIG] Send payload failed");
        break;
      case HTTPC_ERROR_NOT_CONNECTED:
        Serial.println("[CONFIG] Not connected to server");
        break;
      case HTTPC_ERROR_CONNECTION_LOST:
        Serial.println("[CONFIG] Connection lost");
        break;
      case HTTPC_ERROR_NO_STREAM:
        Serial.println("[CONFIG] No stream available");
        break;
      case HTTPC_ERROR_NO_HTTP_SERVER:
        Serial.println("[CONFIG] No HTTP server");
        break;
      case HTTPC_ERROR_TOO_LESS_RAM:
        Serial.println("[CONFIG] Too less RAM");
        break;
      case HTTPC_ERROR_ENCODING:
        Serial.println("[CONFIG] Encoding error");
        break;
      case HTTPC_ERROR_STREAM_WRITE:
        Serial.println("[CONFIG] Stream write error");
        break;
      case HTTPC_ERROR_READ_TIMEOUT:
        Serial.println("[CONFIG] Read timeout");
        break;
      default:
        Serial.println("[CONFIG] Unknown HTTP error");
        break;
    }
    
    String errorString = http.errorToString(httpCode);
    Serial.println("[CONFIG] Error string: " + errorString);
  }
  
  http.end();
  return false;
}

bool fetchConfigFromServer() {
  Serial.println("[CONFIG] Fetching configuration from server");
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[CONFIG] Fetch failed - WiFi not connected");
    return false;
  }

  HTTPClient http;
  String url = String(serverUrl) + "/reader-config?reader_name=" + DEVICE_NAME;
  
  http.begin(url);
  http.addHeader("X-Reader-Key", readerKey);
  http.setTimeout(10000); // 10 second timeout
  
  int httpCode = http.GET();
  Serial.println("[CONFIG] Fetch HTTP code: " + String(httpCode));
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("[CONFIG] Received payload length: " + String(payload.length()));
    
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
      Serial.println("[CONFIG] JSON parse error: " + String(error.c_str()));
      http.end();
      return false;
    }
    
    lastConfigVersion = doc["version"] | lastConfigVersion;
    currentConfigVersion = lastConfigVersion;
    Serial.println("[CONFIG] Updated to version: " + String(lastConfigVersion));
    
    // Update device statuses based on new targets
    deviceStatuses.clear();
    JsonArray targets = doc["targets"];
    Serial.println("[CONFIG] Target devices count: " + String(targets.size()));
    
    for (JsonVariant target : targets) {
      DeviceStatus status;
      status.name = target.as<String>();
      status.isPresent = false;
      status.consecutiveAlerts = 0;
      status.consecutiveStatus = 0;
      status.lastAlertSent = 0;
      status.lastStatusSent = 0;
      status.lastDistance = -1;
      deviceStatuses.push_back(status);
      Serial.println("[CONFIG] Added target device: " + status.name);
    }
    
    if (deviceStatuses.empty()) {
      DeviceStatus status;
      status.name = TARGET_DEVICE_NAME;
      status.isPresent = false;
      status.consecutiveAlerts = 0;
      status.consecutiveStatus = 0;
      status.lastAlertSent = 0;
      status.lastStatusSent = 0;
      status.lastDistance = -1;
      deviceStatuses.push_back(status);
      Serial.println("[CONFIG] Using default target device: " + TARGET_DEVICE_NAME);
    }
    
    JsonObject config = doc["config"];
    if (!config.isNull()) {
      Serial.println("[CONFIG] Processing configuration parameters");
      
      if (!config["kalman"].isNull()) {
        float oldP = kalman_P, oldQ = kalman_Q, oldR = kalman_R, oldInitial = kalman_initial;
        kalman_P = config["kalman"]["P"] | kalman_P;
        kalman_Q = config["kalman"]["Q"] | kalman_Q;
        kalman_R = config["kalman"]["R"] | kalman_R;
        kalman_initial = config["kalman"]["initial"] | kalman_initial;
        
        if (oldP != kalman_P || oldQ != kalman_Q || oldR != kalman_R || oldInitial != kalman_initial) {
          rssiFilter.updateParameters(kalman_Q, kalman_R, kalman_P, kalman_initial);
        }
      }
      
      TX_POWER = config["txPower"] | TX_POWER;
      MAX_DISTANCE = config["maxDistance"] | MAX_DISTANCE;
      SAMPLE_COUNT = config["sampleCount"] | SAMPLE_COUNT;
      SAMPLE_DELAY_MS = config["sampleDelayMs"] | SAMPLE_DELAY_MS;
      PATH_LOSS_EXPONENT = config["pathLossExponent"] | PATH_LOSS_EXPONENT;
      
      Serial.println("[CONFIG] Updated parameters - TX_POWER:" + String(TX_POWER) + 
                    " MAX_DISTANCE:" + String(MAX_DISTANCE) + 
                    " SAMPLE_COUNT:" + String(SAMPLE_COUNT) +
                    " SAMPLE_DELAY:" + String(SAMPLE_DELAY_MS) +
                    " PATH_LOSS:" + String(PATH_LOSS_EXPONENT));
    }
    
    Serial.println("[CONFIG] Configuration fetch successful");
    http.end();
    return true;
  }
  
  Serial.println("[CONFIG] Fetch failed - HTTP code: " + String(httpCode));
  http.end();
  return false;
}

void connectToWiFi() {
  Serial.println("[WIFI] Connecting to " + String(ssid));
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WIFI] Connected successfully");
    Serial.println("[WIFI] IP address: " + WiFi.localIP().toString());
    Serial.println("[WIFI] Signal strength: " + String(WiFi.RSSI()) + " dBm");
  } else {
    Serial.println("[WIFI] Connection failed after " + String(attempts) + " attempts");
    Serial.println("[WIFI] WiFi status: " + String(WiFi.status()));
  }
}

// Simultaneous multi-device scan
void scanAllDevices() {
  if (deviceStatuses.empty()) {
    Serial.println("[SCAN] No target devices configured");
    return;
  }
  
  Serial.println("[SCAN] Starting scan for " + String(deviceStatuses.size()) + " target devices");
  
  // Create a map to store RSSI samples for each device
  std::map<String, std::vector<float>> deviceSamples;
  
  // Initialize sample vectors
  for (auto& deviceStatus : deviceStatuses) {
    deviceSamples[deviceStatus.name] = std::vector<float>();
  }
  
  unsigned long startTime = millis();
  int totalScans = 0;
  
  // Collect samples for all devices simultaneously
  while (totalScans < SAMPLE_COUNT && (millis() - startTime < 30000)) {
    BLEScanResults* foundDevices = pBLEScan->start(1, false);
    
    Serial.println("[SCAN] Found " + String(foundDevices->getCount()) + " BLE devices in scan " + String(totalScans + 1));
    
    for (int i = 0; i < foundDevices->getCount(); i++) {
      BLEAdvertisedDevice device = foundDevices->getDevice(i);
      String deviceName = device.getName().c_str();
      
      // Check if this device is in our target list
      for (auto& deviceStatus : deviceStatuses) {
        if (deviceStatus.name == deviceName) {
          float rssi = device.getRSSI();
          deviceSamples[deviceName].push_back(rssi);
          Serial.println("[SCAN] Target found: " + deviceName + " RSSI:" + String(rssi));
          break;
        }
      }
    }
    
    pBLEScan->clearResults();
    totalScans++;
    delay(SAMPLE_DELAY_MS);
  }
  
  Serial.println("[SCAN] Completed " + String(totalScans) + " scans in " + String(millis() - startTime) + "ms");
  
  // Process results for each device
  for (auto& deviceStatus : deviceStatuses) {
    auto& samples = deviceSamples[deviceStatus.name];
    
    if (!samples.empty()) {
      // Calculate average RSSI
      float rssiSum = 0;
      for (float rssi : samples) {
        rssiSum += rssi;
      }
      float rssiAvg = rssiSum / samples.size();
      float filteredRssi = rssiFilter.update(rssiAvg);
      float filteredDistance = calculateDistance(filteredRssi);
      
      Serial.println("[FILTER] " + deviceStatus.name + " - Raw RSSI avg:" + String(rssiAvg) + 
                    " Filtered RSSI:" + String(filteredRssi) + 
                    " Distance:" + String(filteredDistance) + "m from " + String(samples.size()) + " samples");
      
      deviceStatus.lastDistance = filteredDistance;
      
      // Check if device is within range
      bool currentlyPresent = (filteredDistance <= MAX_DISTANCE && filteredDistance > 0);
      
      if (currentlyPresent) {
        deviceStatus.consecutiveAlerts = 0; // Reset alert counter
        deviceStatus.consecutiveStatus++;
        
        Serial.println("[STATUS] " + deviceStatus.name + " present - consecutive count:" + String(deviceStatus.consecutiveStatus));
        
        // Send heartbeat after consecutive confirmations
        if (deviceStatus.consecutiveStatus >= STATUS_CONFIRMATION_COUNT) {
          unsigned long now = millis();
          unsigned long timeSinceLastStatus = now - deviceStatus.lastStatusSent;
          Serial.println("[STATUS] Ready to send heartbeat - time since last:" + String(timeSinceLastStatus) + "ms (min:" + String(MIN_STATUS_INTERVAL) + "ms)");
          
          if (timeSinceLastStatus > MIN_STATUS_INTERVAL) {
            if (sendHeartbeat(deviceStatus.name, filteredDistance)) {
              deviceStatus.lastStatusSent = now;
            }
          } else {
            Serial.println("[STATUS] Heartbeat skipped - too soon");
          }
          deviceStatus.consecutiveStatus = 0; // Reset after sending
        }
        
        deviceStatus.isPresent = true;
      } else {
        deviceStatus.consecutiveStatus = 0; // Reset status counter
        deviceStatus.consecutiveAlerts++;
        
        Serial.println("[ALERT] " + deviceStatus.name + " out of range - consecutive count:" + String(deviceStatus.consecutiveAlerts));
        
        // Send alert after consecutive confirmations
        if (deviceStatus.consecutiveAlerts >= ALERT_CONFIRMATION_COUNT) {
          unsigned long now = millis();
          unsigned long timeSinceLastAlert = now - deviceStatus.lastAlertSent;
          Serial.println("[ALERT] Ready to send alert - time since last:" + String(timeSinceLastAlert) + "ms (min:" + String(MIN_ALERT_INTERVAL) + "ms)");
          
          if (timeSinceLastAlert > MIN_ALERT_INTERVAL) {
            if (sendAlert(deviceStatus.name, filteredDistance, "out_of_range")) {
              deviceStatus.lastAlertSent = now;
            }
          } else {
            Serial.println("[ALERT] Alert skipped - too soon");
          }
          deviceStatus.consecutiveAlerts = 0; // Reset after sending
        }
        
        deviceStatus.isPresent = false;
      }
      
      Serial.println("[RESULT] " + deviceStatus.name + ": " + String(filteredDistance, 1) + "m " + (currentlyPresent ? "PRESENT" : "ABSENT"));
      
    } else {
      // Device not found
      deviceStatus.consecutiveStatus = 0;
      deviceStatus.consecutiveAlerts++;
      
      Serial.println("[SCAN] " + deviceStatus.name + " not found - consecutive count:" + String(deviceStatus.consecutiveAlerts));
      
      if (deviceStatus.consecutiveAlerts >= ALERT_CONFIRMATION_COUNT) {
        unsigned long now = millis();
        unsigned long timeSinceLastAlert = now - deviceStatus.lastAlertSent;
        Serial.println("[ALERT] Ready to send not found alert - time since last:" + String(timeSinceLastAlert) + "ms");
        
        if (timeSinceLastAlert > MIN_ALERT_INTERVAL) {
          if (sendAlert(deviceStatus.name, -1, "not_found")) {
            deviceStatus.lastAlertSent = now;
          }
        } else {
          Serial.println("[ALERT] Not found alert skipped - too soon");
        }
        deviceStatus.consecutiveAlerts = 0;
      }
      
      deviceStatus.isPresent = false;
      Serial.println("[RESULT] " + deviceStatus.name + ": NOT FOUND");
    }
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("[SYSTEM] ESP32 Asset Reader starting...");
  Serial.println("[SYSTEM] Device name: " + String(DEVICE_NAME));
  
  connectToWiFi();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[SYSTEM] WiFi connected - attempting to fetch initial configuration");
    configFetched = fetchConfigFromServer();
    if (configFetched) {
      Serial.println("[SYSTEM] Initial configuration loaded successfully");
    } else {
      Serial.println("[SYSTEM] Failed to fetch initial configuration - using defaults");
      forceConfigUpdate = true; // Force update on next loop
    }
  } else {
    Serial.println("[SYSTEM] WiFi not connected - using default configuration");
  }
  
  if (deviceStatuses.empty()) {
    DeviceStatus status;
    status.name = TARGET_DEVICE_NAME;
    status.isPresent = false;
    status.consecutiveAlerts = 0;
    status.consecutiveStatus = 0;
    status.lastAlertSent = 0;
    status.lastStatusSent = 0;
    status.lastDistance = -1;
    deviceStatuses.push_back(status);
    Serial.println("[SYSTEM] Using default target device: " + TARGET_DEVICE_NAME);
  }
  
  Serial.println("[BLE] Initializing BLE with device name: " + String(DEVICE_NAME));
  BLEDevice::init(DEVICE_NAME);
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
  pBLEScan->setActiveScan(true);
  Serial.println("[BLE] BLE scanner initialized");
  
  Serial.println("[SYSTEM] Setup complete - starting main loop");
}

void loop() {
  static unsigned long lastVersionCheck = 0;
  static unsigned long lastFullConfigCheck = 0;
  static unsigned long lastNetworkCheck = 0;
  
  // Network connectivity check every 15 seconds
  if (millis() - lastNetworkCheck > 15000) {
    Serial.println("[NETWORK] WiFi Status: " + String(WiFi.status()));
    Serial.println("[NETWORK] IP Address: " + WiFi.localIP().toString());
    Serial.println("[NETWORK] Signal Strength: " + String(WiFi.RSSI()) + " dBm");
    Serial.println("[NETWORK] Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
    
    // Test basic connectivity
    HTTPClient testHttp;
    testHttp.begin("http://httpbin.org/get");
    testHttp.setTimeout(5000);
    int testCode = testHttp.GET();
    Serial.println("[NETWORK] Connectivity test HTTP code: " + String(testCode));
    testHttp.end();
    
    lastNetworkCheck = millis();
  }
  
  // Check WiFi connection status
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WIFI] Connection lost - attempting reconnection");
    connectToWiFi();
  }
  
  // Check config version every 30 seconds
  if (millis() - lastVersionCheck > 30000) {
    Serial.println("[LOOP] Checking for config version updates");
    if (WiFi.status() == WL_CONNECTED) {
      if (checkConfigVersion() || forceConfigUpdate) {
        Serial.println("[LOOP] Config update triggered");
        fetchConfigFromServer();
        forceConfigUpdate = false;
      }
    } else {
      Serial.println("[LOOP] Skipping version check - WiFi not connected");
    }
    lastVersionCheck = millis();
  }
  
  // Fallback: Full config check every 5 minutes
  if (millis() - lastFullConfigCheck > 300000) {
    Serial.println("[LOOP] Performing scheduled full config check");
    if (WiFi.status() == WL_CONNECTED) {
      fetchConfigFromServer();
    } else {
      Serial.println("[LOOP] Skipping full config check - WiFi not connected");
    }
    lastFullConfigCheck = millis();
  }
  
  // Scan all devices simultaneously
  Serial.println("[LOOP] Starting device scan cycle");
  scanAllDevices();
  
  Serial.println("[LOOP] Scan cycle complete - waiting 1 second");
  delay(1000); // 1 second between scan cycles
}