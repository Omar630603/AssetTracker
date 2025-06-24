#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <map>
#include <vector>

// === Configuration ===
const char* ssid = "POCO X6 5G";
const char* password = "12345678";
const char* serverUrl = "https://150.230.8.22/api";
const char* readerKey = "ESP32_READER_KEY";
#define DEVICE_NAME "Asset_Reader_01"

// Dynamic Configuration (Phase 3)
struct Config {
  float txPower = -68.0;
  float pathLossExponent = 2.5;
  float maxDistance = 5.0;
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

// Timing
unsigned long CONFIG_CHECK_INTERVAL = 60000;  // 1 minute
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

// Kalman Filter Implementation
class KalmanFilter {
private:
  float Q, R, P, X;
  bool initialized;
  
public:
  // Default constructor (required for std::map)
  KalmanFilter() 
    : Q(0.1), R(2.0), P(1.0), X(-60.0), initialized(false) {}
  
  // Parameterized constructor
  KalmanFilter(float q, float r, float p, float init) 
    : Q(q), R(r), P(p), X(init), initialized(false) {}
  
  void reconfigure(float q, float r, float p, float init) {
    Q = q; R = r; P = p; X = init;
    initialized = false;
  }
  
  float update(float measurement) {
    if (!initialized) {
      X = measurement;
      initialized = true;
      return X;
    }
    
    P = P + Q;
    float K = P / (P + R);
    X = X + K * (measurement - X);
    P = (1 - K) * P;
    
    return X;
  }
  
  void reset() {
    initialized = false;
  }
};

// Store Kalman filters for each device
std::map<String, KalmanFilter> deviceFilters;

// Check if device should be processed based on discovery mode
bool shouldProcessDevice(const String& deviceName) {
  if (deviceName.isEmpty()) return false;
  
  if (discoveryMode == PATTERN) {
    return deviceName.startsWith(config.assetNamePattern);
  } else { // EXPLICIT mode
    for (const String& target : explicitTargets) {
      if (deviceName == target) {
        return true;
      }
    }
    return false;
  }
}

// Calculate distance from RSSI
float calculateDistance(float rssi) {
  if (rssi == 0 || rssi < -100) return -1.0;
  float distance = pow(10, (config.txPower - rssi) / (10.0 * config.pathLossExponent));
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
        
        // Update configuration
        JsonObject cfg = doc["config"];
        if (!cfg.isNull()) {
          config.txPower = cfg["txPower"] | config.txPower;
          config.pathLossExponent = cfg["pathLossExponent"] | config.pathLossExponent;
          config.maxDistance = cfg["maxDistance"] | config.maxDistance;
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

// Send report to server
bool sendReport(const String& deviceName, float distance, const String& status, float rssi, float filteredRssi) {
  if (WiFi.status() != WL_CONNECTED) return false;
  
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
    Serial.println("[REPORT] Sent successfully");
  } else {
    reportsFailed++;
    Serial.printf("[REPORT] Failed (HTTP %d)\n", httpCode);
  }
  
  https.end();
  delay(10);
  
  return success;
}

// BLE device callback
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {}
};

// Main scanning function
void scanDevices() {
  scanCount++;
  Serial.printf("\n[SCAN #%lu] Starting BLE scan (mode: %s)...\n", 
                scanCount, discoveryMode == PATTERN ? "PATTERN" : "EXPLICIT");
  Serial.printf("[HEAP] Free: %d bytes (largest: %d)\n", 
                ESP.getFreeHeap(), ESP.getMaxAllocHeap());
  
  if (pBLEScan == nullptr) {
    Serial.println("[ERROR] BLE not initialized!");
    return;
  }
  
  BLEScanResults* foundDevices = pBLEScan->start(config.scanTime, false);
  int deviceCount = foundDevices->getCount();
  
  Serial.printf("[SCAN] Found %d devices\n", deviceCount);
  
  int processedCount = 0;
  
  for (int i = 0; i < deviceCount; i++) {
    BLEAdvertisedDevice device = foundDevices->getDevice(i);
    
    String deviceName = "";
    if (device.haveName()) {
      deviceName = device.getName().c_str();
    }
    
    if (!shouldProcessDevice(deviceName)) {
      continue;
    }
    
    processedCount++;
    float rawRssi = device.getRSSI();
    
    // Get or create Kalman filter
    if (deviceFilters.find(deviceName) == deviceFilters.end()) {
      deviceFilters[deviceName] = KalmanFilter(config.kalman.Q, config.kalman.R, 
                                              config.kalman.P, config.kalman.initial);
      Serial.printf("[KALMAN] New filter created for %s\n", deviceName.c_str());
    }
    
    float filteredRssi = deviceFilters[deviceName].update(rawRssi);
    float distance = calculateDistance(filteredRssi);
    String status = (distance > 0 && distance <= config.maxDistance) ? "present" : "out_of_range";
    
    Serial.printf("[DEVICE] %s | RSSI: %.1f -> %.1f | Distance: %.2fm | Status: %s\n",
                  deviceName.c_str(), rawRssi, filteredRssi, distance, status.c_str());
    
    sendReport(deviceName, distance, status, rawRssi, filteredRssi);
    
    delay(50);
  }
  
  Serial.printf("[SCAN] Processed %d matching devices\n", processedCount);
  pBLEScan->clearResults();
  
  // Cleanup if too many filters
  if (deviceFilters.size() > 30) {
    deviceFilters.clear();
    Serial.println("[CLEANUP] Cleared device filters");
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
  
  Serial.println("\n=== ESP32 Asset Reader (Phase 3 - Dynamic Config) ===");
  Serial.printf("Device: %s\n", DEVICE_NAME);
  Serial.printf("Initial heap: %d bytes\n", ESP.getFreeHeap());
  
  // Release Bluetooth Classic memory
  esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
  Serial.printf("After BT classic release: %d bytes\n", ESP.getFreeHeap());
  
  // Initialize BLE
  BLEDevice::init(DEVICE_NAME);
  
  // Configure BLE scan
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
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
  
  // Scan for devices
  scanDevices();
  
  // Print stats every 10 scans
  if (scanCount % 10 == 0) {
    printStats();
  }
  
  // Delay between scans
  delay(2000);
}