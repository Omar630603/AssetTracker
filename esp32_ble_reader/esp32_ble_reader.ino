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
float TX_POWER = -68;  // Calibrated for typical ESP32 at 1 meter
float PATH_LOSS_EXPONENT = 2.5;  // Environmental factor (2.0 for free space, higher for obstacles)
float MAX_DISTANCE = 5.0;
int SAMPLE_COUNT = 10;
int SAMPLE_DELAY_MS = 100;
bool CALIBRATION_MODE = false;  // Set to true to skip server and calibrate

// Enhanced Kalman filter parameters - back to original working values
float kalman_P = 1.0;
float kalman_Q = 0.1;   // Original value that worked
float kalman_R = 2.0;    // Original value that worked  
float kalman_initial = -60.0;

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
  float lastReportedDistance;  // Track what we actually reported
  String lastStatus;
  String lastReportedStatus;   // Track what we actually reported
  bool hasBeenReported;        // Track if device has ever been reported
  float rssiHistory[5];        // Keep history for trend detection
  int historyIndex;
  unsigned long lastSeenTime;
};

BLEScan* pBLEScan;
std::vector<DeviceStatus> deviceStatuses;

// === Timing Configuration ===
const int CONFIRMATION_COUNT = 2;                    // Reduced from 3 for faster response
const unsigned long MIN_HEARTBEAT_INTERVAL = 300000; // 5 minutes between heartbeats
const unsigned long MIN_ALERT_INTERVAL = 30000;      // 30 seconds between alerts (reduced from 60)
const unsigned long VERSION_CHECK_INTERVAL = 60000;  // 1 minute between version checks
const unsigned long NETWORK_CHECK_INTERVAL = 30000;  // 30 seconds network check
const float SIGNIFICANT_DISTANCE_CHANGE = 0.5;       // 0.5 meter change triggers update
const float RSSI_SIGNIFICANT_CHANGE = 5.0;           // 5 dBm change is significant

// Enhanced Kalman Filter with adaptive parameters
class AdaptiveKalmanFilter {
public:
  AdaptiveKalmanFilter(float processNoise, float measurementNoise, float estimatedError, float initialValue) {
    Q = processNoise;
    R = measurementNoise;
    P = estimatedError;
    X = initialValue;
    lastMeasurement = initialValue;
    initialized = false;
  }

  void updateParameters(float processNoise, float measurementNoise, float estimatedError, float initialValue) {
    Q = processNoise;
    R = measurementNoise;
    P = estimatedError;
    X = initialValue;
    initialized = false;
    Serial.println("[KALMAN] Parameters updated - Q:" + String(Q) + " R:" + String(R) + " P:" + String(P) + " Initial:" + String(X));
  }

  float update(float measurement) {
    // Initialize with first measurement
    if (!initialized) {
      X = measurement;
      lastMeasurement = measurement;
      initialized = true;
      return X;
    }
    
    // Check for large jumps and adapt
    float measurementDelta = abs(measurement - lastMeasurement);
    float adaptiveR = R;
    float adaptiveQ = Q;
    
    // If measurement is jumping around a lot, trust it less
    if (measurementDelta > 15.0) {
      adaptiveR = R * 5.0;  // Much less trust in jumpy measurements
      adaptiveQ = Q * 2.0;  // Allow faster state changes
    } else if (measurementDelta > 10.0) {
      adaptiveR = R * 2.0;
      adaptiveQ = Q * 1.5;
    }
    
    // Prediction
    P = P + adaptiveQ;
    
    // Innovation
    float innovation = measurement - X;
    
    // Kalman gain
    K = P / (P + adaptiveR);
    
    // Limit Kalman gain for stability
    K = constrain(K, 0, 0.8);  // Allow up to 0.8 for faster convergence
    
    // Update estimate
    X = X + K * innovation;
    
    // Update error covariance
    P = (1 - K) * P;
    
    // Ensure P doesn't get too small
    P = max(P, 0.1f);
    
    lastMeasurement = measurement;
    
    return X;
  }

  void reset(float initialValue) {
    X = initialValue;
    P = 1.0;
    lastMeasurement = initialValue;
    initialized = true;
  }

private:
  float Q, R, P, K, X;
  float lastMeasurement;
  bool initialized;
};

AdaptiveKalmanFilter rssiFilter(kalman_Q, kalman_R, kalman_P, kalman_initial);

float calculateDistance(float rssi) {
  if (rssi == 0) return -1.0;
  
  // Simple path loss formula: RSSI = TxPower - 10 * n * log10(distance)
  // Rearranged: distance = 10^((TxPower - RSSI) / (10 * n))
  
  float ratio_db = TX_POWER - rssi;
  float ratio_linear = ratio_db / (10.0 * PATH_LOSS_EXPONENT);
  float distance = pow(10, ratio_linear);
  
  // Sanity checks
  if (distance < 0.01) distance = 0.01;
  if (distance > 100) distance = 100;
  
  return distance;
}

// Optimized HTTP request with shorter timeout
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
  http.setTimeout(5000); // Reduced timeout to 5 seconds
  
  // Smaller JSON payload
  StaticJsonDocument<256> doc;
  doc["reader_name"] = DEVICE_NAME;
  doc["device_name"] = deviceName;
  doc["type"] = type;
  doc["status"] = status;
  doc["estimated_distance"] = round(distance * 1000) / 1000.0; // Round to 3 decimals
  
  if (rawRssi != 0) doc["rssi"] = round(rawRssi * 10) / 10.0;
  if (kalmanRssi != 0) doc["kalman_rssi"] = round(kalmanRssi * 100) / 100.0;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial.println("[LOCATION_LOG] Sending: " + jsonString);
  
  int httpCode = http.POST(jsonString);
  bool success = false;
  
  if (httpCode > 0) {
    // Consider 2xx codes as success
    if (httpCode >= 200 && httpCode < 300) {
      Serial.println("[LOCATION_LOG] Success - HTTP " + String(httpCode));
      success = true;
    } else {
      Serial.println("[LOCATION_LOG] Failed - HTTP " + String(httpCode));
    }
  } else {
    Serial.println("[LOCATION_LOG] HTTP Error: " + http.errorToString(httpCode));
  }
  
  http.end();
  return success;
}

// Check if we need to send update based on changes
bool needsUpdate(DeviceStatus& deviceStatus, const String& currentStatus, float currentDistance) {
  unsigned long now = millis();
  
  // First time detection - always report
  if (!deviceStatus.hasBeenReported) {
    Serial.println("[UPDATE] First time detection for " + deviceStatus.name);
    return true;
  }
  
  // Status change - always report (device appeared/disappeared)
  if (currentStatus != deviceStatus.lastReportedStatus) {
    Serial.println("[UPDATE] Status changed from " + deviceStatus.lastReportedStatus + " to " + currentStatus);
    return true;
  }
  
  // Check for significant distance change (only for present devices)
  if (currentStatus == "present" && deviceStatus.lastReportedDistance > 0) {
    float distanceChange = abs(currentDistance - deviceStatus.lastReportedDistance);
    if (distanceChange >= SIGNIFICANT_DISTANCE_CHANGE) {
      Serial.println("[UPDATE] Significant distance change: " + String(distanceChange) + "m");
      return true;
    }
  }
  
  // Regular heartbeat interval for present devices
  unsigned long timeSinceLastReport = now - deviceStatus.lastReportSent;
  if (currentStatus == "present" && timeSinceLastReport >= MIN_HEARTBEAT_INTERVAL) {
    Serial.println("[UPDATE] Heartbeat interval reached: " + String(timeSinceLastReport/1000) + "s");
    return true;
  }
  
  // Alert re-send interval for missing/out of range devices
  if (currentStatus != "present" && timeSinceLastReport >= MIN_ALERT_INTERVAL) {
    Serial.println("[UPDATE] Alert interval reached: " + String(timeSinceLastReport/1000) + "s");
    return true;
  }
  
  return false;
}

// Check for RSSI trend to detect movement
bool isMoving(DeviceStatus& deviceStatus, float currentRssi) {
  // Add to history
  deviceStatus.rssiHistory[deviceStatus.historyIndex] = currentRssi;
  deviceStatus.historyIndex = (deviceStatus.historyIndex + 1) % 5;
  
  // Need full history
  if (deviceStatus.rssiHistory[4] == 0) return false;
  
  // Calculate variance
  float sum = 0, mean = 0;
  for (int i = 0; i < 5; i++) {
    sum += deviceStatus.rssiHistory[i];
  }
  mean = sum / 5.0;
  
  float variance = 0;
  for (int i = 0; i < 5; i++) {
    variance += pow(deviceStatus.rssiHistory[i] - mean, 2);
  }
  variance /= 5.0;
  
  // High variance indicates movement
  return variance > 25.0; // 5 dBm standard deviation
}

void scanAllDevices() {
  if (deviceStatuses.empty()) {
    Serial.println("[SCAN] No target devices configured");
    return;
  }
  
  Serial.println("[SCAN] Scanning for " + String(deviceStatuses.size()) + " devices");
  
  std::map<String, std::vector<float>> deviceSamples;
  
  // Initialize sample vectors for all devices
  for (auto& deviceStatus : deviceStatuses) {
    deviceSamples[deviceStatus.name] = std::vector<float>();
  }
  
  unsigned long startTime = millis();
  int actualSamples = 0;
  
  // Adaptive sampling - reduce samples if devices are stable
  int samplesToTake = SAMPLE_COUNT;
  bool allStable = true;
  for (auto& deviceStatus : deviceStatuses) {
    if (isMoving(deviceStatus, deviceStatus.lastDistance) || !deviceStatus.hasBeenReported) {
      allStable = false;
      break;
    }
  }
  if (allStable) {
    samplesToTake = SAMPLE_COUNT / 2;
  }
  
  // Collect samples for ALL devices simultaneously
  while (actualSamples < samplesToTake && (millis() - startTime < 10000)) {
    BLEScanResults* foundDevices = pBLEScan->start(1, false);
    
    for (int i = 0; i < foundDevices->getCount(); i++) {
      BLEAdvertisedDevice device = foundDevices->getDevice(i);
      String deviceName = device.getName().c_str();
      
      // Check if this is one of our target devices
      for (auto& deviceStatus : deviceStatuses) {
        if (deviceStatus.name == deviceName) {
          float rssi = device.getRSSI();
          deviceSamples[deviceName].push_back(rssi);
          break;
        }
      }
    }
    
    pBLEScan->clearResults();
    actualSamples++;
    
    if (actualSamples < samplesToTake) {
      delay(SAMPLE_DELAY_MS);
    }
  }
  
  Serial.println("[SCAN] Completed " + String(actualSamples) + " scans in " + String(millis() - startTime) + "ms");
  
  // Process results for each device and send individual reports as needed
  for (auto& deviceStatus : deviceStatuses) {
    auto& samples = deviceSamples[deviceStatus.name];
    String currentStatus;
    float rawRssi = 0;
    float filteredRssi = 0;
    float filteredDistance = -1;
    
    if (!samples.empty()) {
      // Remove outliers before averaging
      if (samples.size() > 2) {
        std::sort(samples.begin(), samples.end());
        samples.erase(samples.begin());
        samples.pop_back();
      }
      
      // Calculate average RSSI
      float rssiSum = 0;
      for (float rssi : samples) {
        rssiSum += rssi;
      }
      rawRssi = rssiSum / samples.size();
      
      // Reset Kalman filter for devices that were lost for a while
      if (!deviceStatus.isPresent && (millis() - deviceStatus.lastSeenTime) > 30000) {
        rssiFilter.reset(rawRssi);
        Serial.println("[FILTER] Reset Kalman for " + deviceStatus.name);
      }
      
      filteredRssi = rssiFilter.update(rawRssi);
      filteredDistance = calculateDistance(filteredRssi);
      
      // Debug output for calibration
      if (!CALIBRATION_MODE) {
        Serial.println("[DEBUG] " + deviceStatus.name + " Raw RSSI:" + String(rawRssi, 1) + 
                      " Filtered:" + String(filteredRssi, 1) + " Distance:" + String(filteredDistance, 2) + "m");
      }
      
      deviceStatus.lastSeenTime = millis();
      deviceStatus.lastDistance = filteredDistance;
      
      // Determine status with hysteresis
      if (filteredDistance <= MAX_DISTANCE && filteredDistance > 0) {
        currentStatus = "present";
        deviceStatus.isPresent = true;
      } else if (filteredDistance > MAX_DISTANCE * 1.2) {
        currentStatus = "out_of_range";
        deviceStatus.isPresent = false;
      } else {
        // In hysteresis zone - keep previous status
        currentStatus = deviceStatus.lastStatus.isEmpty() ? "out_of_range" : deviceStatus.lastStatus;
      }
      
      // Check for movement
      bool moving = isMoving(deviceStatus, rawRssi);
      if (moving && deviceStatus.hasBeenReported) {
        Serial.println("[MOVEMENT] " + deviceStatus.name + " is moving");
      }
      
    } else {
      // Device not found
      currentStatus = "not_found";
      deviceStatus.isPresent = false;
      deviceStatus.lastDistance = -1;
      filteredDistance = -1;
    }
    
    // Update consecutive detection counter
    if (currentStatus == deviceStatus.lastStatus) {
      deviceStatus.consecutiveDetections++;
    } else {
      deviceStatus.consecutiveDetections = 1;
    }
    
    deviceStatus.lastStatus = currentStatus;
    
    Serial.println("[STATUS] " + deviceStatus.name + " " + currentStatus + 
                  " dist:" + String(filteredDistance, 2) + "m" +
                  " consec:" + String(deviceStatus.consecutiveDetections));
    
    // Check if we should send a report (after confirmation count reached)
    if (deviceStatus.consecutiveDetections >= CONFIRMATION_COUNT) {
      if (needsUpdate(deviceStatus, currentStatus, filteredDistance)) {
        String reportType = (currentStatus == "present") ? "heartbeat" : "alert";
        
        if (sendLocationLog(deviceStatus.name, filteredDistance, reportType, currentStatus, rawRssi, filteredRssi)) {
          deviceStatus.lastReportSent = millis();
          deviceStatus.lastReportedDistance = filteredDistance;
          deviceStatus.lastReportedStatus = currentStatus;
          deviceStatus.hasBeenReported = true;
          Serial.println("[REPORT] Successfully sent " + reportType + " for " + deviceStatus.name);
        } else {
          Serial.println("[REPORT] Failed to send " + reportType + " for " + deviceStatus.name);
        }
      } else {
        Serial.println("[REPORT] No update needed for " + deviceStatus.name);
      }
    }
  }
}

bool checkAndFetchConfig() {
  Serial.println("[CONFIG] Checking configuration");
  
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  HTTPClient http;
  String url = String(serverUrl) + "/reader-config?reader_name=" + DEVICE_NAME;
  
  http.begin(url);
  http.addHeader("X-Reader-Key", readerKey);
  http.setTimeout(5000); // Reduced timeout
  
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
      unsigned long serverVersion = doc["version"] | 0;
      
      if (serverVersion != lastConfigVersion || forceConfigUpdate) {
        applyConfiguration(doc);
        lastConfigVersion = serverVersion;
        currentConfigVersion = serverVersion;
        forceConfigUpdate = false;
        Serial.println("[CONFIG] Updated to version " + String(serverVersion));
      }
    }
    
    http.end();
    return true;
  }
  
  http.end();
  return false;
}

void applyConfiguration(DynamicJsonDocument& doc) {
  Serial.println("[CONFIG] Applying configuration");
  
  // Update device statuses
  deviceStatuses.clear();
  JsonArray targets = doc["targets"];
  
  for (JsonVariant target : targets) {
    DeviceStatus status;
    status.name = target.as<String>();
    status.isPresent = false;
    status.consecutiveDetections = 0;
    status.lastReportSent = 0;
    status.lastDistance = -1;
    status.lastReportedDistance = -1;
    status.lastStatus = "";
    status.lastReportedStatus = "";
    status.hasBeenReported = false;
    status.historyIndex = 0;
    status.lastSeenTime = 0;
    memset(status.rssiHistory, 0, sizeof(status.rssiHistory));
    deviceStatuses.push_back(status);
    Serial.println("[CONFIG] Added device: " + status.name);
  }
  
  if (deviceStatuses.empty()) {
    DeviceStatus status;
    status.name = TARGET_DEVICE_NAME;
    status.isPresent = false;
    status.consecutiveDetections = 0;
    status.lastReportSent = 0;
    status.lastDistance = -1;
    status.lastReportedDistance = -1;
    status.lastStatus = "";
    status.lastReportedStatus = "";
    status.hasBeenReported = false;
    status.historyIndex = 0;
    status.lastSeenTime = 0;
    memset(status.rssiHistory, 0, sizeof(status.rssiHistory));
    deviceStatuses.push_back(status);
  }
  
  JsonObject config = doc["config"];
  if (!config.isNull()) {
    if (!config["kalman"].isNull()) {
      kalman_P = config["kalman"]["P"] | kalman_P;
      kalman_Q = config["kalman"]["Q"] | kalman_Q;
      kalman_R = config["kalman"]["R"] | kalman_R;
      kalman_initial = config["kalman"]["initial"] | kalman_initial;
      rssiFilter.updateParameters(kalman_Q, kalman_R, kalman_P, kalman_initial);
    }
    
    TX_POWER = config["txPower"] | TX_POWER;
    MAX_DISTANCE = config["maxDistance"] | MAX_DISTANCE;
    SAMPLE_COUNT = config["sampleCount"] | SAMPLE_COUNT;
    SAMPLE_DELAY_MS = config["sampleDelayMs"] | SAMPLE_DELAY_MS;
    PATH_LOSS_EXPONENT = config["pathLossExponent"] | PATH_LOSS_EXPONENT;
  }
}

void connectToWiFi() {
  Serial.println("[WIFI] Connecting to " + String(ssid));
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(250);
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WIFI] Connected - IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("[WIFI] Connection failed");
  }
}

void setup() {
  Serial.begin(115200); // Increased baud rate
  Serial.println("\n[SYSTEM] ESP32 Asset Reader v2.0");
  Serial.println("[SYSTEM] Device: " + String(DEVICE_NAME));
  
  if (CALIBRATION_MODE) {
    Serial.println("\n================== CALIBRATION MODE ==================");
    Serial.println("Place tag at EXACTLY 1 meter from reader");
    Serial.println("Current TX_POWER: " + String(TX_POWER));
    Serial.println("Look for 'Avg RSSI at 1m' in the output");
    Serial.println("Update TX_POWER to match that value");
    Serial.println("Then set CALIBRATION_MODE = false");
    Serial.println("=====================================================\n");
    
    // In calibration mode, skip WiFi and use default target
    DeviceStatus status;
    status.name = TARGET_DEVICE_NAME;
    status.isPresent = false;
    status.consecutiveDetections = 0;
    status.lastReportSent = 0;
    status.lastDistance = -1;
    status.lastReportedDistance = -1;
    status.lastStatus = "";
    status.lastReportedStatus = "";
    status.hasBeenReported = false;
    status.historyIndex = 0;
    status.lastSeenTime = 0;
    memset(status.rssiHistory, 0, sizeof(status.rssiHistory));
    deviceStatuses.push_back(status);
  } else {
    // Normal mode - connect to WiFi and fetch config
    connectToWiFi();
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("[CONFIG] Fetching initial configuration...");
      configFetched = checkAndFetchConfig();
      if (configFetched) {
        Serial.println("[CONFIG] Initial configuration loaded");
      } else {
        Serial.println("[CONFIG] Using default configuration");
      }
    }
    
    // Ensure default device if needed
    if (deviceStatuses.empty()) {
      DeviceStatus status;
      status.name = TARGET_DEVICE_NAME;
      status.isPresent = false;
      status.consecutiveDetections = 0;
      status.lastReportSent = 0;
      status.lastDistance = -1;
      status.lastReportedDistance = -1;
      status.lastStatus = "";
      status.lastReportedStatus = "";
      status.hasBeenReported = false;
      status.historyIndex = 0;
      status.lastSeenTime = 0;
      memset(status.rssiHistory, 0, sizeof(status.rssiHistory));
      deviceStatuses.push_back(status);
    }
  }
  
  // Initialize BLE
  BLEDevice::init(DEVICE_NAME);
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
  pBLEScan->setActiveScan(true);
  
  Serial.println("[SYSTEM] Ready\n");
}

void loop() {
  static unsigned long lastVersionCheck = 0;
  static unsigned long lastNetworkCheck = 0;
  static int wifiRetries = 0;
  
  if (!CALIBRATION_MODE) {
    // Network check
    if (millis() - lastNetworkCheck > NETWORK_CHECK_INTERVAL) {
      if (WiFi.status() != WL_CONNECTED) {
        wifiRetries++;
        if (wifiRetries <= 3) {
          Serial.println("[WIFI] Connection lost - attempting reconnect");
          connectToWiFi();
        }
      } else {
        wifiRetries = 0;
      }
      lastNetworkCheck = millis();
    }
    
    // Config check - every 60 seconds
    if (millis() - lastVersionCheck > VERSION_CHECK_INTERVAL) {
      Serial.println("[CONFIG] Checking for updates (60s interval)");
      if (WiFi.status() == WL_CONNECTED) {
        if (checkAndFetchConfig()) {
          Serial.println("[CONFIG] Check complete");
        } else {
          Serial.println("[CONFIG] Check failed");
        }
      } else {
        Serial.println("[CONFIG] Skipped - no WiFi");
      }
      lastVersionCheck = millis();
    }
  } else {
    // Calibration mode - show running average
    static float rssiSum = 0;
    static int rssiCount = 0;
    
    BLEScanResults* foundDevices = pBLEScan->start(1, false);
    
    for (int i = 0; i < foundDevices->getCount(); i++) {
      BLEAdvertisedDevice device = foundDevices->getDevice(i);
      if (device.getName() == TARGET_DEVICE_NAME) {
        float rssi = device.getRSSI();
        rssiSum += rssi;
        rssiCount++;
        
        if (rssiCount % 10 == 0) {
          float avgRssi = rssiSum / rssiCount;
          Serial.println("\n[CALIBRATION] Samples: " + String(rssiCount));
          Serial.println("[CALIBRATION] Avg RSSI at 1m: " + String(avgRssi, 1));
          Serial.println("[CALIBRATION] Set TX_POWER = " + String(avgRssi, 0));
          Serial.println("[CALIBRATION] Current distance calc: " + 
                        String(calculateDistance(avgRssi), 2) + "m\n");
        }
      }
    }
    
    pBLEScan->clearResults();
    delay(500);
    return;
  }
  
  // Main scanning
  scanAllDevices();
  
  // Adaptive delay based on device state
  bool anyDevicePresent = false;
  for (auto& device : deviceStatuses) {
    if (device.isPresent) {
      anyDevicePresent = true;
      break;
    }
  }
  
  // Shorter delay when devices are present
  delay(anyDevicePresent ? 1000 : 2000);
}