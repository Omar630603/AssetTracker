#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <map>
#include <vector>
#include <algorithm>

// === Configuration ===
const char* ssid = "POCO X6 5G";
const char* password = "12345678";
const char* serverUrl = "https://150.230.8.22/api";
const char* readerKey = "ESP32_READER_KEY";
#define DEVICE_NAME "Asset_Reader_01"

// Dynamic Configuration
struct Config {
  float txPower = -68.0;
  float pathLossExponent = 2.5;
  float maxDistance = 5.0;
  int sampleCount = 5;
  int sampleDelayMs = 100;
  int scanTime = 3;
  String assetNamePattern = "Asset_";
  
  struct KalmanParams {
    float Q = 0.1;
    float R = 2.0;
    float P = 1.0;
    float initial = -60.0;
  } kalman;
};

Config config;

// Discovery Mode
enum DiscoveryMode {
  PATTERN,
  EXPLICIT
};

DiscoveryMode discoveryMode = PATTERN;
std::vector<String> explicitTargets;

// Device tracking structure
struct DeviceData {
  String name;
  float rawRssi;
  float filteredRssi;
  float distance;
  String status;
  unsigned long lastSeen;
  int sampleCount;
  float rssiSum;
};

// Timing
unsigned long CONFIG_CHECK_INTERVAL = 60000;
unsigned long lastConfigCheck = 0;
unsigned long currentConfigVersion = 0;

// Stats
unsigned long scanCount = 0;
unsigned long reportsSent = 0;
unsigned long reportsSuccess = 0;
unsigned long reportsFailed = 0;

// BLE
BLEScan* pBLEScan = nullptr;

// Keep a single global WiFiClientSecure to reuse
WiFiClientSecure* secureClient = nullptr;

// Advanced Kalman Filter Implementation
class KalmanFilter {
private:
  float Q, R, P, X, K;
  bool initialized;
  unsigned long lastUpdate;
  float innovation;
  
public:
  // Default constructor
  KalmanFilter() 
    : Q(0.1), R(2.0), P(1.0), X(-60.0), K(0), initialized(false), lastUpdate(0), innovation(0) {}
  
  // Parameterized constructor
  KalmanFilter(float q, float r, float p, float init) 
    : Q(q), R(r), P(p), X(init), K(0), initialized(false), lastUpdate(0), innovation(0) {}
  
  void reconfigure(float q, float r, float p, float init) {
    Q = q; R = r; P = p; X = init;
    initialized = false;
    K = 0;
    innovation = 0;
  }
  
  float update(float measurement) {
    unsigned long now = millis();
    
    if (!initialized) {
      X = measurement;
      initialized = true;
      lastUpdate = now;
      return X;
    }
    
    // Time-based process noise adjustment
    float dt = (now - lastUpdate) / 1000.0; // seconds
    float Qdt = Q * (1.0 + dt); // Increase process noise with time
    
    // Prediction step
    P = P + Qdt;
    
    // Update step
    K = P / (P + R);  // Kalman gain
    innovation = measurement - X;  // Innovation
    X = X + K * innovation;  // State update
    P = (1 - K) * P;  // Error covariance update
    
    lastUpdate = now;
    return X;
  }
  
  void reset() {
    initialized = false;
    P = 1.0;
    K = 0;
    innovation = 0;
  }
  
  float getInnovation() const { return innovation; }
  float getGain() const { return K; }
  bool isStable() const { return initialized && abs(innovation) < 5.0; }
};

// Store Kalman filters for each device
std::map<String, KalmanFilter> deviceFilters;

// Multi-sample collection
std::map<String, DeviceData> scannedDevices;

// Check if device should be processed based on discovery mode
bool shouldProcessDevice(const String& deviceName) {
  if (deviceName.isEmpty()) return false;
  
  if (discoveryMode == PATTERN) {
    return deviceName.startsWith(config.assetNamePattern);
  } else { // EXPLICIT mode
    return std::find(explicitTargets.begin(), explicitTargets.end(), deviceName) != explicitTargets.end();
  }
}

// Calculate distance from RSSI with environmental compensation
float calculateDistance(float rssi) {
  if (rssi == 0 || rssi < -100) return -1.0;
  
  // Environmental factor (can be made configurable)
  float environmentalFactor = 1.0;
  
  float distance = pow(10, (config.txPower - rssi) / (10.0 * config.pathLossExponent)) * environmentalFactor;
  return constrain(distance, 0.01, 100.0);
}

// Connect to WiFi
void connectWiFi() {
  Serial.println("\n[WIFI] Connecting to " + String(ssid));
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WIFI] Connected!");
    Serial.println("[WIFI] IP: " + WiFi.localIP().toString());
    Serial.println("[WIFI] RSSI: " + String(WiFi.RSSI()) + " dBm");
  } else {
    Serial.println("\n[WIFI] Failed to connect!");
  }
}

// Initialize secure client
void initSecureClient() {
  if (secureClient == nullptr) {
    secureClient = new WiFiClientSecure();
    secureClient->setInsecure();
    Serial.println("[HTTPS] Secure client initialized");
  }
}

// Fetch and apply configuration from server
bool fetchAndApplyConfig() {
  if (WiFi.status() != WL_CONNECTED) return false;
  
  initSecureClient();
  
  HTTPClient https;
  String url = String(serverUrl) + "/reader-config?reader_name=" + DEVICE_NAME;
  
  if (!https.begin(*secureClient, url)) {
    Serial.println("[CONFIG] Failed to begin HTTPS");
    return false;
  }
  
  https.addHeader("X-Reader-Key", readerKey);
  https.setTimeout(10000);
  https.setReuse(false);
  
  int httpCode = https.GET();
  bool success = false;
  
  if (httpCode == 200) {
    String payload = https.getString();
    DynamicJsonDocument doc(2048);
    
    if (deserializeJson(doc, payload) == DeserializationError::Ok) {
      unsigned long serverVersion = doc["version"] | 0;
      
      if (serverVersion != currentConfigVersion) {
        currentConfigVersion = serverVersion;
        
        // Update discovery mode
        String mode = doc["discovery_mode"] | "pattern";
        if (mode == "explicit") {
          discoveryMode = EXPLICIT;
          Serial.println("[CONFIG] Discovery mode: EXPLICIT");
          
          // Load explicit targets
          explicitTargets.clear();
          JsonArray targets = doc["targets"];
          for (JsonVariant target : targets) {
            explicitTargets.push_back(target.as<String>());
            Serial.println("[CONFIG] Target: " + target.as<String>());
          }
          Serial.printf("[CONFIG] Loaded %d explicit targets\n", explicitTargets.size());
        } else {
          discoveryMode = PATTERN;
          Serial.println("[CONFIG] Discovery mode: PATTERN");
        }
        
        // Update configuration (aligned with Laravel model)
        JsonObject cfg = doc["config"];
        if (!cfg.isNull()) {
          config.txPower = cfg["txPower"] | config.txPower;
          config.pathLossExponent = cfg["pathLossExponent"] | config.pathLossExponent;
          config.maxDistance = cfg["maxDistance"] | config.maxDistance;
          config.sampleCount = cfg["sampleCount"] | config.sampleCount;
          config.sampleDelayMs = cfg["sampleDelayMs"] | config.sampleDelayMs;
          config.scanTime = cfg["scanTime"] | config.scanTime;
          config.assetNamePattern = cfg["assetNamePattern"] | config.assetNamePattern;
          
          // Update Kalman parameters
          if (cfg.containsKey("kalman")) {
            JsonObject kalman = cfg["kalman"];
            config.kalman.Q = kalman["Q"] | config.kalman.Q;
            config.kalman.R = kalman["R"] | config.kalman.R;
            config.kalman.P = kalman["P"] | config.kalman.P;
            config.kalman.initial = kalman["initial"] | config.kalman.initial;
            
            // Reconfigure all existing filters
            for (auto& pair : deviceFilters) {
              pair.second.reconfigure(config.kalman.Q, config.kalman.R, 
                                     config.kalman.P, config.kalman.initial);
            }
            Serial.println("[CONFIG] Kalman filters reconfigured");
          }
          
          // Print updated config
          Serial.println("[CONFIG] Updated configuration:");
          Serial.printf("  TX Power: %.1f dBm\n", config.txPower);
          Serial.printf("  Path Loss Exp: %.1f\n", config.pathLossExponent);
          Serial.printf("  Max Distance: %.1fm\n", config.maxDistance);
          Serial.printf("  Sample Count: %d\n", config.sampleCount);
          Serial.printf("  Sample Delay: %dms\n", config.sampleDelayMs);
          Serial.printf("  Scan Time: %d sec\n", config.scanTime);
          Serial.printf("  Pattern: %s\n", config.assetNamePattern.c_str());
          Serial.printf("  Kalman Q=%.2f R=%.2f\n", config.kalman.Q, config.kalman.R);
        }
        
        success = true;
        Serial.printf("[CONFIG] Applied version %lu\n", serverVersion);
      } else {
        Serial.println("[CONFIG] No update needed");
      }
    }
  } else {
    Serial.printf("[CONFIG] HTTP error: %d\n", httpCode);
  }
  
  https.end();
  return success;
}

// Send batch reports for multiple devices
bool sendBatchReports() {
  if (scannedDevices.empty() || WiFi.status() != WL_CONNECTED) return false;
  
  int successCount = 0;
  
  for (auto& pair : scannedDevices) {
    DeviceData& device = pair.second;
    
    // Skip if no valid samples
    if (device.sampleCount == 0) continue;
    
    // Average RSSI from multiple samples
    float avgRssi = device.rssiSum / device.sampleCount;
    
    // Apply Kalman filter
    if (deviceFilters.find(device.name) == deviceFilters.end()) {
      deviceFilters[device.name] = KalmanFilter(config.kalman.Q, config.kalman.R, 
                                               config.kalman.P, config.kalman.initial);
    }
    
    device.filteredRssi = deviceFilters[device.name].update(avgRssi);
    device.distance = calculateDistance(device.filteredRssi);
    device.status = (device.distance > 0 && device.distance <= config.maxDistance) ? "present" : "out_of_range";
    
    // Send individual report
    if (sendReport(device.name, device.distance, device.status, avgRssi, device.filteredRssi)) {
      successCount++;
    }
    
    delay(50); // Small delay between reports
  }
  
  return successCount > 0;
}

// Send single device report
bool sendReport(const String& deviceName, float distance, const String& status, float rssi, float filteredRssi) {
  reportsSent++;
  
  initSecureClient();
  
  HTTPClient https;
  String url = String(serverUrl) + "/reader-log";
  
  if (!https.begin(*secureClient, url)) {
    Serial.println("[REPORT] Failed to begin HTTPS");
    reportsFailed++;
    return false;
  }
  
  https.addHeader("Content-Type", "application/json");
  https.addHeader("X-Reader-Key", readerKey);
  https.setTimeout(5000);
  https.setReuse(false);
  
  char jsonBuffer[384];
  snprintf(jsonBuffer, sizeof(jsonBuffer),
    "{\"reader_name\":\"%s\",\"device_name\":\"%s\",\"type\":\"%s\",\"status\":\"%s\","
    "\"estimated_distance\":%.2f,\"rssi\":%.1f,\"kalman_rssi\":%.1f}",
    DEVICE_NAME, deviceName.c_str(), 
    (status == "present") ? "heartbeat" : "alert",
    status.c_str(),
    round(distance * 100) / 100.0,
    round(rssi * 10) / 10.0,
    round(filteredRssi * 10) / 10.0
  );
  
  int httpCode = https.POST(jsonBuffer);
  bool success = (httpCode >= 200 && httpCode < 300);
  
  if (success) {
    reportsSuccess++;
    Serial.printf("[REPORT] %s: %.1f->%.1f dBm, %.2fm (%s) - OK\n", 
                  deviceName.c_str(), rssi, filteredRssi, distance, status.c_str());
  } else {
    reportsFailed++;
    Serial.printf("[REPORT] %s: Failed (HTTP %d)\n", deviceName.c_str(), httpCode);
  }
  
  https.end();
  return success;
}

// Advanced multi-sample scanning
void performMultiSampleScan() {
  scanCount++;
  Serial.printf("\n[SCAN #%lu] Multi-sample scan (mode: %s, samples: %d)...\n", 
                scanCount, discoveryMode == PATTERN ? "PATTERN" : "EXPLICIT", config.sampleCount);
  Serial.printf("[HEAP] Free: %d bytes (largest: %d)\n", 
                ESP.getFreeHeap(), ESP.getMaxAllocHeap());
  
  if (pBLEScan == nullptr) {
    Serial.println("[ERROR] BLE not initialized!");
    return;
  }
  
  // Clear previous scan data
  scannedDevices.clear();
  
  // Perform multiple sample scans
  for (int sample = 0; sample < config.sampleCount; sample++) {
    Serial.printf("[SAMPLE %d/%d] Scanning...\n", sample + 1, config.sampleCount);
    
    BLEScanResults* foundDevices = pBLEScan->start(config.scanTime, false);
    int deviceCount = foundDevices->getCount();
    
    for (int i = 0; i < deviceCount; i++) {
      BLEAdvertisedDevice device = foundDevices->getDevice(i);
      
      String deviceName = "";
      if (device.haveName()) {
        deviceName = device.getName().c_str();
      }
      
      if (!shouldProcessDevice(deviceName)) {
        continue;
      }
      
      float rssi = device.getRSSI();
      
      // Add to or update device data
      if (scannedDevices.find(deviceName) == scannedDevices.end()) {
        DeviceData data;
        data.name = deviceName;
        data.rssiSum = rssi;
        data.sampleCount = 1;
        data.lastSeen = millis();
        scannedDevices[deviceName] = data;
      } else {
        scannedDevices[deviceName].rssiSum += rssi;
        scannedDevices[deviceName].sampleCount++;
        scannedDevices[deviceName].lastSeen = millis();
      }
    }
    
    pBLEScan->clearResults();
    
    // Delay between samples (except after last sample)
    if (sample < config.sampleCount - 1) {
      delay(config.sampleDelayMs);
    }
  }
  
  // Process and report all scanned devices
  Serial.printf("[SCAN] Found %d unique matching devices\n", scannedDevices.size());

  if (discoveryMode == EXPLICIT) {
    // For explicit mode, report found devices and send "not_found" for missing
    std::vector<String> foundNames;
    for (const auto& pair : scannedDevices) {
      foundNames.push_back(pair.first);
    }
    if (!scannedDevices.empty()) {
      sendBatchReports();
    }
    // Report for each explicit target, if not found, send "not_found"
    for (const String& target : explicitTargets) {
      if (std::find(foundNames.begin(), foundNames.end(), target) == foundNames.end()) {
        // Not found, so report "not_found"
        sendReport(target, -1.0, "not_found", -100.0, -100.0);
        delay(50);
      }
    }
  } else {
    // Pattern mode: report as usual
    if (!scannedDevices.empty()) {
      sendBatchReports();
    }
  }

  // Cleanup old filters
  if (deviceFilters.size() > 30) {
    // Remove filters for devices not seen recently
    auto it = deviceFilters.begin();
    while (it != deviceFilters.end()) {
      bool found = false;
      for (const auto& pair : scannedDevices) {
        if (pair.first == it->first) {
          found = true;
          break;
        }
      }
      if (!found) {
        it = deviceFilters.erase(it);
      } else {
        ++it;
      }
    }
    Serial.printf("[CLEANUP] Active filters: %d\n", deviceFilters.size());
  }
}

// Print statistics
void printStats() {
  Serial.println("\n========== STATISTICS ==========");
  Serial.printf("Uptime: %lu seconds\n", millis() / 1000);
  Serial.printf("Config version: %lu\n", currentConfigVersion);
  Serial.printf("Discovery mode: %s\n", discoveryMode == PATTERN ? "PATTERN" : "EXPLICIT");
  if (discoveryMode == PATTERN) {
    Serial.printf("Pattern: %s*\n", config.assetNamePattern.c_str());
  } else {
    Serial.printf("Targets: %d devices\n", explicitTargets.size());
  }
  Serial.printf("Scans: %lu, Reports: %lu (OK: %lu, Fail: %lu)\n", 
                scanCount, reportsSent, reportsSuccess, reportsFailed);
  float successRate = (reportsSent > 0) ? (float(reportsSuccess) / float(reportsSent) * 100.0) : 0;
  Serial.printf("Success rate: %.1f%%\n", successRate);
  Serial.printf("Active filters: %d\n", deviceFilters.size());
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("WiFi RSSI: %d dBm\n", WiFi.RSSI());
  Serial.println("================================\n");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== ESP32 Asset Reader ===");
  Serial.printf("Device: %s\n", DEVICE_NAME);
  Serial.printf("Initial heap: %d bytes\n", ESP.getFreeHeap());
  
  // Release Bluetooth Classic memory
  esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
  Serial.printf("After BT classic release: %d bytes\n", ESP.getFreeHeap());
  
  // Initialize BLE
  BLEDevice::init(DEVICE_NAME);
  
  // Configure BLE scan with optimizations
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
  pBLEScan->setActiveScan(true);

  Serial.printf("After BLE init: %d bytes\n", ESP.getFreeHeap());
  
  // Connect WiFi
  connectWiFi();
  Serial.printf("After WiFi: %d bytes\n", ESP.getFreeHeap());
  
  // Initialize secure client
  initSecureClient();
  Serial.printf("After HTTPS init: %d bytes\n", ESP.getFreeHeap());
  
  // Fetch initial configuration
  Serial.println("\n[CONFIG] Fetching initial configuration...");
  if (fetchAndApplyConfig()) {
    Serial.println("[CONFIG] Configuration applied successfully");
  } else {
    Serial.println("[CONFIG] Using default configuration");
  }
  
  Serial.println("[SYSTEM] Ready!\n");
  delay(2000);
}

void loop() {
  // Check WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WIFI] Connection lost, reconnecting...");
    connectWiFi();
    delay(5000);
    return;
  }
  
  // Periodic config update
  if (millis() - lastConfigCheck > CONFIG_CHECK_INTERVAL) {
    Serial.println("\n[CONFIG] Checking for updates...");
    fetchAndApplyConfig();
    lastConfigCheck = millis();
  }
  
  // Perform multi-sample scan
  performMultiSampleScan();
  
  // Print stats every 10 scans
  if (scanCount % 10 == 0) {
    printStats();
  }
  
  // Delay between scan cycles
  delay(2000);
}