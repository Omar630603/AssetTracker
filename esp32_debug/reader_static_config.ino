#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <map>

// === Configuration ===
const char* ssid = "POCO X6 5G";
const char* password = "12345678";
const char* serverUrl = "https://150.230.8.22/api";
const char* readerKey = "ESP32_READER_KEY";
#define DEVICE_NAME "Asset_Reader_01"

// Static Configuration (Phase 2)
const float TX_POWER = -68.0;
const float PATH_LOSS_EXPONENT = 2.5;
const float MAX_DISTANCE = 5.0;
const int SCAN_TIME = 3;
const String ASSET_NAME_PATTERN = "Asset_";

// Kalman filter static parameters
const float KALMAN_Q = 0.1;
const float KALMAN_R = 2.0;
const float KALMAN_P = 1.0;
const float KALMAN_INIT = -60.0;

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

// Simple Kalman Filter Implementation
class KalmanFilter {
private:
  float Q, R, P, X;
  bool initialized;
  
public:
  KalmanFilter(float q = KALMAN_Q, float r = KALMAN_R, float p = KALMAN_P, float init = KALMAN_INIT) 
    : Q(q), R(r), P(p), X(init), initialized(false) {}
  
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
    P = KALMAN_P;
    X = KALMAN_INIT;
  }
};

// Store Kalman filters for each device
std::map<String, KalmanFilter> deviceFilters;

// Calculate distance from RSSI
float calculateDistance(float rssi) {
  if (rssi == 0 || rssi < -100) return -1.0;
  float distance = pow(10, (TX_POWER - rssi) / (10.0 * PATH_LOSS_EXPONENT));
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
    secureClient->setInsecure(); // Skip certificate verification
    Serial.println("[HTTPS] Secure client initialized");
  }
}

// Check version from API
bool checkConfigVersion() {
  if (WiFi.status() != WL_CONNECTED) return false;
  
  initSecureClient();
  
  HTTPClient https;
  String url = String(serverUrl) + "/reader-config?reader_name=" + DEVICE_NAME + "&version_check=true";
  
  if (!https.begin(*secureClient, url)) {
    Serial.println("[CONFIG] Failed to begin HTTPS");
    return false;
  }
  
  https.addHeader("X-Reader-Key", readerKey);
  https.setTimeout(5000);
  https.setReuse(false); // Don't reuse connection
  
  int httpCode = https.GET();
  bool needsUpdate = false;
  
  if (httpCode == 200) {
    String payload = https.getString();
    StaticJsonDocument<256> doc;
    
    if (deserializeJson(doc, payload) == DeserializationError::Ok) {
      unsigned long serverVersion = doc["version"] | 0;
      if (serverVersion != currentConfigVersion) {
        Serial.printf("[CONFIG] Version changed: %lu -> %lu\n", currentConfigVersion, serverVersion);
        currentConfigVersion = serverVersion;
        needsUpdate = true;
      }
    }
  } else {
    Serial.printf("[CONFIG] HTTP error: %d\n", httpCode);
  }
  
  https.end();
  
  return needsUpdate;
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
  
  // Use static buffer to avoid heap allocation
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
    if (httpCode == -1) {
      Serial.printf("[REPORT] Connection failed - Heap: %d, Largest: %d\n", 
                    ESP.getFreeHeap(), ESP.getMaxAllocHeap());
    }
  }
  
  https.end();
  
  // Small delay to ensure cleanup
  delay(10);
  
  return success;
}

// BLE device callback
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    // Just for counting in this callback
  }
};

// Main scanning function
void scanDevices() {
  scanCount++;
  Serial.printf("\n[SCAN #%lu] Starting BLE scan...\n", scanCount);
  Serial.printf("[HEAP] Free: %d bytes (largest: %d)\n", 
                ESP.getFreeHeap(), ESP.getMaxAllocHeap());
  
  if (pBLEScan == nullptr) {
    Serial.println("[ERROR] BLE not initialized!");
    return;
  }
  
  BLEScanResults* foundDevices = pBLEScan->start(SCAN_TIME, false);
  int deviceCount = foundDevices->getCount();
  
  Serial.printf("[SCAN] Found %d devices\n", deviceCount);
  
  int processedCount = 0;
  
  for (int i = 0; i < deviceCount; i++) {
    BLEAdvertisedDevice device = foundDevices->getDevice(i);
    
    String deviceName = "";
    if (device.haveName()) {
      deviceName = device.getName().c_str();
    }
    
    if (!deviceName.startsWith(ASSET_NAME_PATTERN)) {
      continue;
    }
    
    processedCount++;
    float rawRssi = device.getRSSI();
    
    // Get or create Kalman filter
    if (deviceFilters.find(deviceName) == deviceFilters.end()) {
      deviceFilters[deviceName] = KalmanFilter(KALMAN_Q, KALMAN_R, KALMAN_P, KALMAN_INIT);
      Serial.printf("[KALMAN] New filter created for %s\n", deviceName.c_str());
    }
    
    float filteredRssi = deviceFilters[deviceName].update(rawRssi);
    float distance = calculateDistance(filteredRssi);
    String status = (distance > 0 && distance <= MAX_DISTANCE) ? "present" : "out_of_range";
    
    Serial.printf("[DEVICE] %s | RSSI: %.1f -> %.1f | Distance: %.2fm | Status: %s\n",
                  deviceName.c_str(), rawRssi, filteredRssi, distance, status.c_str());
    
    // Send report
    sendReport(deviceName, distance, status, rawRssi, filteredRssi);
    
    delay(50); // Small delay between reports
  }
  
  Serial.printf("[SCAN] Processed %d matching devices\n", processedCount);
  pBLEScan->clearResults();
  
  // Limit stored filters
  if (deviceFilters.size() > 20) {
    deviceFilters.clear();
    Serial.println("[CLEANUP] Cleared device filters");
  }
}

// Print statistics
void printStats() {
  Serial.println("\n========== STATISTICS ==========");
  Serial.printf("Uptime: %lu seconds\n", millis() / 1000);
  Serial.printf("Scans completed: %lu\n", scanCount);
  Serial.printf("Reports sent: %lu (Success: %lu, Failed: %lu)\n", 
                reportsSent, reportsSuccess, reportsFailed);
  float successRate = (reportsSent > 0) ? (float(reportsSuccess) / float(reportsSent) * 100.0) : 0;
  Serial.printf("Success rate: %.1f%%\n", successRate);
  Serial.printf("Active filters: %d\n", deviceFilters.size());
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("Largest block: %d bytes\n", ESP.getMaxAllocHeap());
  Serial.printf("WiFi RSSI: %d dBm\n", WiFi.RSSI());
  Serial.println("================================\n");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== ESP32 Asset Reader (Phase 2 - Optimized) ===");
  Serial.printf("Device: %s\n", DEVICE_NAME);
  Serial.printf("Initial heap: %d bytes\n", ESP.getFreeHeap());
  
  // Release Bluetooth Classic memory
  esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
  Serial.printf("After BT classic release: %d bytes\n", ESP.getFreeHeap());
  
  // Initialize BLE with proper device name
  BLEDevice::init(DEVICE_NAME);
  
  // Configure BLE scan
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
  pBLEScan->setActiveScan(true); // Need active scan to get device names
  
  Serial.printf("After BLE init: %d bytes\n", ESP.getFreeHeap());
  
  // Connect WiFi
  connectWiFi();
  Serial.printf("After WiFi: %d bytes\n", ESP.getFreeHeap());
  
  // Initialize secure client
  initSecureClient();
  Serial.printf("After HTTPS init: %d bytes\n", ESP.getFreeHeap());
  
  // Check initial config version
  Serial.println("\n[CONFIG] Checking version...");
  if (checkConfigVersion()) {
    Serial.println("[CONFIG] New version available (will use static config for now)");
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
  
  // Periodic version check
  if (millis() - lastConfigCheck > CONFIG_CHECK_INTERVAL) {
    if (checkConfigVersion()) {
      Serial.println("[CONFIG] Version updated (static config still in use)");
    }
    lastConfigCheck = millis();
  }
  
  // Scan for devices
  scanDevices();
  
  // Print stats every 10 scans
  if (scanCount % 10 == 0) {
    printStats();
  }
  
  // Small delay between scans
  delay(2000);
}