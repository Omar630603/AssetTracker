#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// === Wi-Fi Configuration ===
const char* ssid = "POCO X6 5G";
const char* password = "12345678";
const char* serverUrl = "http://150.230.8.22/api"; 
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

// === Device Status Tracking ===
struct DeviceStatus {
  String name;
  bool isPresent;
  int consecutiveDetections;
  unsigned long lastReportSent;
  float lastDistance;
  String lastStatus;
};

BLEScan* pBLEScan;
std::vector<DeviceStatus> deviceStatuses;

// === Timing Configuration ===
const int CONFIRMATION_COUNT = 3;                 // Reports after 3 consecutive detections
const unsigned long MIN_HEARTBEAT_INTERVAL = 300000; // 5 minutes between heartbeats
const unsigned long MIN_ALERT_INTERVAL = 60000;      // 1 minute between alerts
const unsigned long VERSION_CHECK_INTERVAL = 60000;  // 1 minute between version checks
const unsigned long NETWORK_CHECK_INTERVAL = 15000;  // 15 seconds network check

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

// Unified HTTP response handler
bool handleHttpResponse(HTTPClient& http, const String& operation) {
  int httpCode = http.GET();
  Serial.println("[" + operation + "] HTTP code: " + String(httpCode));
  
  if (httpCode > 0) {
    String response = http.getString();
    Serial.println("[" + operation + "] Response length: " + String(response.length()));
    
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
      Serial.println("[" + operation + "] Success");
      return true;
    } else {
      Serial.println("[" + operation + "] Failed - HTTP code: " + String(httpCode));
      Serial.println("[" + operation + "] Response: " + response);
      return false;
    }
  } else {
    Serial.println("[" + operation + "] HTTP Error: " + String(httpCode));
    logHttpError(httpCode, operation);
    return false;
  }
}

bool handleHttpPostResponse(HTTPClient& http, const String& operation) {
  int httpCode = http.POST("");
  Serial.println("[" + operation + "] HTTP code: " + String(httpCode));
  
  if (httpCode > 0) {
    String response = http.getString();
    Serial.println("[" + operation + "] Response length: " + String(response.length()));
    
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
      Serial.println("[" + operation + "] Success");
      return true;
    } else {
      Serial.println("[" + operation + "] Failed - HTTP code: " + String(httpCode));
      Serial.println("[" + operation + "] Response: " + response);
      return false;
    }
  } else {
    Serial.println("[" + operation + "] HTTP Error: " + String(httpCode));
    logHttpError(httpCode, operation);
    return false;
  }
}

void logHttpError(int httpCode, const String& operation) {
  Serial.println("[" + operation + "] Error Details:");
  switch(httpCode) {
    case HTTPC_ERROR_CONNECTION_REFUSED:
      Serial.println("[" + operation + "] Connection refused by server");
      break;
    case HTTPC_ERROR_SEND_HEADER_FAILED:
      Serial.println("[" + operation + "] Send header failed");
      break;
    case HTTPC_ERROR_SEND_PAYLOAD_FAILED:
      Serial.println("[" + operation + "] Send payload failed");
      break;
    case HTTPC_ERROR_NOT_CONNECTED:
      Serial.println("[" + operation + "] Not connected to server");
      break;
    case HTTPC_ERROR_CONNECTION_LOST:
      Serial.println("[" + operation + "] Connection lost");
      break;
    case HTTPC_ERROR_READ_TIMEOUT:
      Serial.println("[" + operation + "] Read timeout");
      break;
    default:
      Serial.println("[" + operation + "] Unknown HTTP error");
      break;
  }
}

// Unified location log sender (replaces both sendAlert and sendHeartbeat)
bool sendLocationLog(const String& deviceName, float distance, const String& type, const String& status, float rawRssi = 0, float kalmanRssi = 0) {
  Serial.println("[LOCATION_LOG] Sending " + type + " for " + deviceName + " status:" + status + " distance:" + String(distance));
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[LOCATION_LOG] Failed - WiFi not connected");
    return false;
  }

  HTTPClient http;
  String url = String(serverUrl) + "/reader-log";
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Reader-Key", readerKey);
  http.setTimeout(10000);
  
  DynamicJsonDocument doc(512);
  doc["reader_name"] = DEVICE_NAME;
  doc["device_name"] = deviceName;
  doc["type"] = type;
  doc["status"] = status;
  doc["estimated_distance"] = distance;
  doc["timestamp"] = millis();
  
  // Include RSSI data if available
  if (rawRssi != 0) doc["rssi"] = rawRssi;
  if (kalmanRssi != 0) doc["kalman_rssi"] = kalmanRssi;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial.println("[LOCATION_LOG] Sending JSON: " + jsonString);
  
  int httpCode = http.POST(jsonString);
  bool success = handleHttpPostResponse(http, "LOCATION_LOG");
  http.end();
  return success;
}

// Enhanced config check that returns config in same response if update needed
bool checkAndFetchConfig() {
  Serial.println("[CONFIG] Checking version and fetching config if needed");
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[CONFIG] Failed - WiFi not connected");
    return false;
  }

  HTTPClient http;
  String url = String(serverUrl) + "/reader-config?reader_name=" + DEVICE_NAME;
  
  // Always fetch full config to simplify logic
  http.begin(url);
  http.addHeader("X-Reader-Key", readerKey);
  http.setTimeout(10000);
  
  int httpCode = http.GET();
  
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
    
    unsigned long serverVersion = doc["version"] | 0;
    
    // Check if update is needed
    if (serverVersion != lastConfigVersion || forceConfigUpdate) {
      Serial.println("[CONFIG] Update needed - Server:" + String(serverVersion) + " Local:" + String(lastConfigVersion));
      applyConfiguration(doc);
      lastConfigVersion = serverVersion;
      currentConfigVersion = serverVersion;
      forceConfigUpdate = false;
      Serial.println("[CONFIG] Configuration updated successfully");
    } else {
      Serial.println("[CONFIG] Configuration up to date");
    }
    
    http.end();
    return true;
  }
  
  Serial.println("[CONFIG] Failed - HTTP code: " + String(httpCode));
  logHttpError(httpCode, "CONFIG");
  http.end();
  return false;
}

// Separate function to apply configuration
void applyConfiguration(DynamicJsonDocument& doc) {
  Serial.println("[CONFIG] Applying new configuration");
  
  // Update device statuses based on new targets
  deviceStatuses.clear();
  JsonArray targets = doc["targets"];
  Serial.println("[CONFIG] Target devices count: " + String(targets.size()));
  
  for (JsonVariant target : targets) {
    DeviceStatus status;
    status.name = target.as<String>();
    status.isPresent = false;
    status.consecutiveDetections = 0;
    status.lastReportSent = 0;
    status.lastDistance = -1;
    status.lastStatus = "";
    deviceStatuses.push_back(status);
    Serial.println("[CONFIG] Added target device: " + status.name);
  }
  
  if (deviceStatuses.empty()) {
    DeviceStatus status;
    status.name = TARGET_DEVICE_NAME;
    status.isPresent = false;
    status.consecutiveDetections = 0;
    status.lastReportSent = 0;
    status.lastDistance = -1;
    status.lastStatus = "";
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

// Determine appropriate report type and interval based on status
bool shouldSendReport(DeviceStatus& deviceStatus, const String& currentStatus) {
  unsigned long now = millis();
  String reportType;
  unsigned long minInterval;
  
  if (currentStatus == "present") {
    reportType = "heartbeat";
    minInterval = MIN_HEARTBEAT_INTERVAL;
  } else {
    reportType = "alert";
    minInterval = MIN_ALERT_INTERVAL;
  }
  
  unsigned long timeSinceLastReport = now - deviceStatus.lastReportSent;
  
  Serial.println("[REPORT] " + deviceStatus.name + " " + reportType + " check - consecutive:" + 
                String(deviceStatus.consecutiveDetections) + " time since last:" + 
                String(timeSinceLastReport) + "ms (min:" + String(minInterval) + "ms)");
  
  return (deviceStatus.consecutiveDetections >= CONFIRMATION_COUNT && 
          timeSinceLastReport > minInterval);
}

void scanAllDevices() {
  if (deviceStatuses.empty()) {
    Serial.println("[SCAN] No target devices configured");
    return;
  }
  
  Serial.println("[SCAN] Starting scan for " + String(deviceStatuses.size()) + " target devices");
  
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
    String currentStatus;
    float rawRssi = 0;
    float filteredRssi = 0;
    float filteredDistance = -1;
    
    if (!samples.empty()) {
      // Calculate average RSSI
      float rssiSum = 0;
      for (float rssi : samples) {
        rssiSum += rssi;
      }
      rawRssi = rssiSum / samples.size();
      filteredRssi = rssiFilter.update(rawRssi);
      filteredDistance = calculateDistance(filteredRssi);
      
      Serial.println("[FILTER] " + deviceStatus.name + " - Raw RSSI avg:" + String(rawRssi) + 
                    " Filtered RSSI:" + String(filteredRssi) + 
                    " Distance:" + String(filteredDistance) + "m from " + String(samples.size()) + " samples");
      
      deviceStatus.lastDistance = filteredDistance;
      
      // Determine status
      if (filteredDistance <= MAX_DISTANCE && filteredDistance > 0) {
        currentStatus = "present";
        deviceStatus.isPresent = true;
      } else {
        currentStatus = "out_of_range";
        deviceStatus.isPresent = false;
      }
    } else {
      // Device not found
      currentStatus = "not_found";
      deviceStatus.isPresent = false;
      Serial.println("[SCAN] " + deviceStatus.name + " not found");
    }
    
    // Update consecutive detection counter
    if (currentStatus == deviceStatus.lastStatus) {
      deviceStatus.consecutiveDetections++;
    } else {
      deviceStatus.consecutiveDetections = 1; // Reset to 1 for new status
      deviceStatus.lastStatus = currentStatus;
    }
    
    Serial.println("[STATUS] " + deviceStatus.name + " status:" + currentStatus + 
                  " consecutive:" + String(deviceStatus.consecutiveDetections));
    
    // Check if we should send a report
    if (shouldSendReport(deviceStatus, currentStatus)) {
      String reportType = (currentStatus == "present") ? "heartbeat" : "alert";
      
      if (sendLocationLog(deviceStatus.name, filteredDistance, reportType, currentStatus, rawRssi, filteredRssi)) {
        deviceStatus.lastReportSent = millis();
        deviceStatus.consecutiveDetections = 0; // Reset after successful send
      }
    }
    
    currentStatus.toUpperCase();
    Serial.println("[RESULT] " + deviceStatus.name + ": " + String(filteredDistance, 1) + "m " + currentStatus);
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("[SYSTEM] ESP32 Asset Reader starting...");
  Serial.println("[SYSTEM] Device name: " + String(DEVICE_NAME));
  
  connectToWiFi();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[SYSTEM] WiFi connected - fetching initial configuration");
    configFetched = checkAndFetchConfig();
    if (configFetched) {
      Serial.println("[SYSTEM] Initial configuration loaded successfully");
    } else {
      Serial.println("[SYSTEM] Failed to fetch initial configuration - using defaults");
      forceConfigUpdate = true;
    }
  } else {
    Serial.println("[SYSTEM] WiFi not connected - using default configuration");
  }
  
  // Ensure we have at least one target device
  if (deviceStatuses.empty()) {
    DeviceStatus status;
    status.name = TARGET_DEVICE_NAME;
    status.isPresent = false;
    status.consecutiveDetections = 0;
    status.lastReportSent = 0;
    status.lastDistance = -1;
    status.lastStatus = "";
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
  static unsigned long lastNetworkCheck = 0;
  
  // Network connectivity check
  if (millis() - lastNetworkCheck > NETWORK_CHECK_INTERVAL) {
    Serial.println("[NETWORK] WiFi Status: " + String(WiFi.status()));
    Serial.println("[NETWORK] IP Address: " + WiFi.localIP().toString());
    Serial.println("[NETWORK] Signal Strength: " + String(WiFi.RSSI()) + " dBm");
    Serial.println("[NETWORK] Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
    lastNetworkCheck = millis();
  }
  
  // Check WiFi connection status
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WIFI] Connection lost - attempting reconnection");
    connectToWiFi();
  }
  
  // Check config version with 1-minute interval
  if (millis() - lastVersionCheck > VERSION_CHECK_INTERVAL) {
    Serial.println("[LOOP] Checking for config updates");
    if (WiFi.status() == WL_CONNECTED) {
      checkAndFetchConfig();
    } else {
      Serial.println("[LOOP] Skipping config check - WiFi not connected");
    }
    lastVersionCheck = millis();
  }
  
  // Scan all devices
  Serial.println("[LOOP] Starting device scan cycle");
  scanAllDevices();
  
  Serial.println("[LOOP] Scan cycle complete - waiting 1 second");
  delay(1000);
}