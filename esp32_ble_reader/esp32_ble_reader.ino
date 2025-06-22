// ========================
// BLE READER (ESP32 READER DEVICE)
// ========================
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <mbedtls/aes.h>
#include <map>
#include <set>
#include <algorithm>

// Network Configuration
const char* ssid = "POCO X6 5G";
const char* password = "12345678";
const char* serverUrl = "https://150.230.8.22/api"; 
const char* readerKey = "ESP32_READER_KEY";
const char* SECRET_KEY = "ESP32_TAG_SECRET_KEY";
#define DEVICE_NAME "Asset_Reader_01"
#define COMPANY_ID 0xFFFF

// Default Parameters
float TX_POWER = -68;
float PATH_LOSS_EXPONENT = 2.5;
float MAX_DISTANCE = 5.0;
int SAMPLE_COUNT = 10;
int SAMPLE_DELAY_MS = 100;
bool CALIBRATION_MODE = false;

// Discovery Configuration
String DISCOVERY_MODE = "pattern";
String ASSET_NAME_PATTERN = "Asset_";
String TARGET_DEVICE_NAME = "Asset_Tag_01";

// Kalman Filter Parameters
float kalman_P = 1.0, kalman_Q = 0.1, kalman_R = 2.0, kalman_initial = -60.0;

// Timing Constants (ms)
const int CONFIRMATION_COUNT = 2;
const unsigned long MIN_HEARTBEAT_INTERVAL = 300000;
const unsigned long MIN_ALERT_INTERVAL = 30000;
const unsigned long VERSION_CHECK_INTERVAL = 60000;
const unsigned long NETWORK_CHECK_INTERVAL = 30000;
const float SIGNIFICANT_DISTANCE_CHANGE = 0.5;

// Global State
BLEScan* pBLEScan;
bool configFetched = false;
unsigned long lastConfigVersion = 0;
std::map<String, uint8_t[4]> authTokenCache;

// Device tracking structure
struct DeviceStatus {
  String name;
  bool isPresent = false;
  int consecutiveDetections = 0;
  unsigned long lastReportSent = 0;
  float lastDistance = -1;
  float lastReportedDistance = -1;
  String lastStatus = "";
  String lastReportedStatus = "";
  bool hasBeenReported = false;
  float rssiHistory[5] = {0};
  int historyIndex = 0;
  unsigned long lastSeenTime = 0;
};
std::vector<DeviceStatus> deviceStatuses;

// Adaptive Kalman Filter for RSSI smoothing
class AdaptiveKalmanFilter {
  float Q, R, P, K, X, lastMeasurement;
  bool initialized = false;
  
public:
  AdaptiveKalmanFilter(float q, float r, float p, float x) : Q(q), R(r), P(p), X(x) {}
  
  void updateParameters(float q, float r, float p, float x) {
    Q = q; R = r; P = p; X = x;
    initialized = false;
  }
  
  float update(float measurement) {
    if (!initialized) {
      X = lastMeasurement = measurement;
      initialized = true;
      return X;
    }
    
    float delta = abs(measurement - lastMeasurement);
    float adaptiveR = R * (delta > 15 ? 5 : delta > 10 ? 2 : 1);
    float adaptiveQ = Q * (delta > 15 ? 2 : delta > 10 ? 1.5 : 1);
    
    P += adaptiveQ;
    K = constrain(P / (P + adaptiveR), 0, 0.8);
    X += K * (measurement - X);
    P = max((1 - K) * P, 0.1f);
    lastMeasurement = measurement;
    
    return X;
  }
  
  void reset(float val) { X = lastMeasurement = val; P = 1.0; initialized = true; }
};
AdaptiveKalmanFilter rssiFilter(kalman_Q, kalman_R, kalman_P, kalman_initial);

// Calculate distance from RSSI using path loss model
float calculateDistance(float rssi) {
  if (rssi == 0) return -1.0;
  float distance = pow(10, (TX_POWER - rssi) / (10.0 * PATH_LOSS_EXPONENT));
  return constrain(distance, 0.01, 100);
}

// Generate authentication token using AES encryption
void generateAuthToken(const String& data, uint8_t* output) {
  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  
  uint8_t input[16] = {0}, key[16];
  memcpy(input, data.c_str(), min(data.length(), (size_t)16));
  memcpy(key, SECRET_KEY, 16);
  
  mbedtls_aes_setkey_enc(&aes, key, 128);
  mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, input, output);
  mbedtls_aes_free(&aes);
}

// --- HEX HELPERS ---
uint8_t hexCharToNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return 0;
}
std::vector<uint8_t> hexToBytes(const String& hex) {
  std::vector<uint8_t> bytes;
  for (unsigned int i = 0; i + 1 < hex.length(); i += 2) {
    bytes.push_back((hexCharToNibble(hex[i]) << 4) | hexCharToNibble(hex[i + 1]));
  }
  return bytes;
}

// Validate BLE device authentication
bool validateAssetTag(BLEAdvertisedDevice& device) {
  String name = device.getName();
  
  // Name validation
  if (name.length() < 7 || !name.startsWith(ASSET_NAME_PATTERN)) return false;
  
  // Check cache
  auto cached = authTokenCache.find(name);
  if (cached != authTokenCache.end()) return true;
  
  // Validate manufacturer data
  if (!device.haveManufacturerData()) return false;
  
  String mfgStr = device.getManufacturerData();
  std::vector<uint8_t> mfgBytes = hexToBytes(mfgStr);
  if (mfgBytes.size() < 8) return false;
  
  uint16_t companyId = mfgBytes[0] | (mfgBytes[1] << 8);
  if (companyId != COMPANY_ID || mfgBytes[2] != 0x01) return false;
  
  // Verify authentication token
  uint8_t expectedToken[16];
  generateAuthToken(name, expectedToken);

  if (memcmp(&mfgBytes[4], expectedToken, 4) == 0) {
    memcpy(authTokenCache[name], expectedToken, 4);
    return true;
  }
  
  return false;
}

// Send location data to server
bool sendLocationLog(const String& deviceName, float distance, const String& type, 
                    const String& status, float rawRssi, float kalmanRssi, bool auth) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[HTTP] Not connected to WiFi, cannot send log!");
    return false;
  }
  size_t before = ESP.getFreeHeap();
  Serial.printf("[HTTP] Reporting %s: %s, Status: %s, Dist: %.2fm\n", type.c_str(), deviceName.c_str(), status.c_str(), distance);

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, String(serverUrl) + "/reader-log");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Reader-Key", readerKey);
  http.setTimeout(5000);

  StaticJsonDocument<256> doc;
  doc["reader_name"] = DEVICE_NAME;
  doc["device_name"] = deviceName;
  doc["type"] = type;
  doc["status"] = status;
  doc["estimated_distance"] = round(distance * 1000) / 1000.0;
  doc["authenticated"] = auth;
  if (rawRssi != 0) doc["rssi"] = round(rawRssi * 10) / 10.0;
  if (kalmanRssi != 0) doc["kalman_rssi"] = round(kalmanRssi * 100) / 100.0;

  String json;
  serializeJson(doc, json);

  int code = http.POST(json);
  String response = http.getString();
  http.end();
  client.stop();
  Serial.printf("[HTTP DEBUG] POST code: %d, error: %s\n", code, http.errorToString(code).c_str());
  Serial.printf("[LOG] %s %s: %s (HTTP %d)\n", type.c_str(), deviceName.c_str(), (code >= 200 && code < 300) ? "sent" : "failed", code);
  Serial.printf("Heap after sendLocationLog: %d\n", ESP.getFreeHeap());
  return (code >= 200 && code < 300);
}

// Check if device status needs reporting
bool needsUpdate(DeviceStatus& dev, const String& status, float distance) {
  unsigned long now = millis();
  
  if (!dev.hasBeenReported || status != dev.lastReportedStatus) return true;
  
  if (status == "present") {
    if (abs(distance - dev.lastReportedDistance) >= SIGNIFICANT_DISTANCE_CHANGE) return true;
    if (now - dev.lastReportSent >= MIN_HEARTBEAT_INTERVAL) return true;
  } else {
    if (now - dev.lastReportSent >= MIN_ALERT_INTERVAL) return true;
  }
  
  return false;
}

// Scan and process BLE devices
void scanAllDevices() {
  Serial.println("[SCAN] Scanning for BLE tags...");
  if (DISCOVERY_MODE == "explicit" && deviceStatuses.empty()) return;
  
  std::map<String, std::vector<float>> samples;
  unsigned long startTime = millis();
  int scans = 0;
  
  // Collect RSSI samples
  while (scans < SAMPLE_COUNT && (millis() - startTime < 10000)) {
    BLEScanResults* results = pBLEScan->start(1, false);
    
    for (int i = 0; i < results->getCount(); i++) {
      BLEAdvertisedDevice device = results->getDevice(i);
      String name = device.getName();
      if (name.isEmpty()) continue;
      
      bool process = false;
      bool authenticated = true;
      
      if (DISCOVERY_MODE == "pattern") {
        if ((authenticated = validateAssetTag(device))) {
          Serial.println("[SCAN] Tag authenticated: " + name);
          process = true;
          
          // Auto-add new authenticated devices
          auto it = std::find_if(deviceStatuses.begin(), deviceStatuses.end(),
                                [&name](const DeviceStatus& d) { return d.name == name; });
          if (it == deviceStatuses.end()) {
            deviceStatuses.push_back({name});
            Serial.println("[SCAN] New tag found: " + name);
          }
        }
      } else {
        // Explicit mode - check target list
        auto it = std::find_if(deviceStatuses.begin(), deviceStatuses.end(),
                              [&name](const DeviceStatus& d) { return d.name == name; });
        if (it != deviceStatuses.end()) {
          process = true;
          if (device.haveManufacturerData()) {
            authenticated = validateAssetTag(device);
          }
        }
      }
      
      if (process) {
        samples[name].push_back(device.getRSSI());
      }
    }
    
    pBLEScan->clearResults();
    scans++;
    if (scans < SAMPLE_COUNT) delay(SAMPLE_DELAY_MS);
  }
  
  // Process each tracked device
  for (auto& dev : deviceStatuses) {
    auto& rssiSamples = samples[dev.name];
    String status;
    float rawRssi = 0, filteredRssi = 0, distance = -1;
    bool authenticated = (DISCOVERY_MODE == "pattern");
    
    if (!rssiSamples.empty()) {
      // Remove outliers
      if (rssiSamples.size() > 2) {
        std::sort(rssiSamples.begin(), rssiSamples.end());
        rssiSamples.erase(rssiSamples.begin());
        rssiSamples.pop_back();
      }
      
      // Average RSSI
      float sum = 0;
      for (float v : rssiSamples) sum += v;
      rawRssi = sum / rssiSamples.size();
      
      // Reset filter if device was lost
      if (!dev.isPresent && (millis() - dev.lastSeenTime > 30000)) {
        rssiFilter.reset(rawRssi);
      }
      
      filteredRssi = rssiFilter.update(rawRssi);
      distance = calculateDistance(filteredRssi);
      
      dev.lastSeenTime = millis();
      dev.lastDistance = distance;
      
      // Determine status with hysteresis
      if (distance <= MAX_DISTANCE && distance > 0) {
        status = "present";
        dev.isPresent = true;
      } else if (distance > MAX_DISTANCE * 1.2) {
        status = "out_of_range";
        dev.isPresent = false;
      } else {
        status = dev.lastStatus.isEmpty() ? "out_of_range" : dev.lastStatus;
      }
    } else {
      status = "not_found";
      dev.isPresent = false;
      dev.lastDistance = distance = -1;
    }

    Serial.printf("[SCAN] %s | Status: %s | RSSI: raw=%.1f, filtered=%.1f | Dist: %.2fm\n",
    dev.name.c_str(), status.c_str(), rawRssi, filteredRssi, distance);
    
    // Update consecutive detections
    dev.consecutiveDetections = (status == dev.lastStatus) ? dev.consecutiveDetections + 1 : 1;
    dev.lastStatus = status;
    
    // Report if confirmed and needed
    if (dev.consecutiveDetections >= CONFIRMATION_COUNT && needsUpdate(dev, status, distance)) {
      String type = (status == "present") ? "heartbeat" : "alert";

      Serial.printf("[DEVICE] Status change: %s | %s | Dist: %.2fm | RSSI: %.1f\n", dev.name.c_str(), status.c_str(), distance, rawRssi);

      if (sendLocationLog(dev.name, distance, type, status, rawRssi, filteredRssi, authenticated)) {
        dev.lastReportSent = millis();
        dev.lastReportedDistance = distance;
        dev.lastReportedStatus = status;
        dev.hasBeenReported = true;
      } else if (!rssiSamples.empty()) {
        Serial.printf("[SCAN] %s | No update needed (status stable or not significant)\n", dev.name.c_str());
      }
    }
    
    if (CALIBRATION_MODE) {
      Serial.printf("[CAL] %s Raw:%.1f Filtered:%.1f Dist:%.2fm\n", 
                    dev.name.c_str(), rawRssi, filteredRssi, distance);
    }
  }
}

// Fetch configuration from server
bool fetchConfig() {
  Serial.println("[CONFIG] Fetching configuration from server...");
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[CONFIG] Not connected to WiFi!");
    return false;
  }
  
  size_t before = ESP.getFreeHeap();

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, String(serverUrl) + "/reader-config?reader_name=" + DEVICE_NAME);
  http.addHeader("X-Reader-Key", readerKey);
  http.setTimeout(5000);

  int code = http.GET();
  String payload = http.getString(); // Save to variable before .end()
  http.end();
  client.stop();
  Serial.printf("[CONFIG] HTTP code: %d\n", code);
  Serial.println("[CONFIG] Response: " + payload);
  Serial.printf("Heap after fetchConfig: %d\n", ESP.getFreeHeap());
  if (code != HTTP_CODE_OK) return false;

  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, payload);
  http.end();
  client.stop();

  if (error) {
    Serial.println("[CONFIG] JSON Parse Error!");
    return false;
  }

  unsigned long version = doc["version"] | 0;
  if (version == lastConfigVersion) {
    Serial.println("[CONFIG] Up-to-date (no changes).");
    return true;
  }
  
  // Apply new configuration
  DISCOVERY_MODE = doc["discovery_mode"] | "pattern";
  deviceStatuses.clear();
  authTokenCache.clear();
  
  JsonObject config = doc["config"];
  if (!config.isNull()) {
    ASSET_NAME_PATTERN = config["assetNamePattern"] | ASSET_NAME_PATTERN;
    TX_POWER = config["txPower"] | TX_POWER;
    MAX_DISTANCE = config["maxDistance"] | MAX_DISTANCE;
    SAMPLE_COUNT = config["sampleCount"] | SAMPLE_COUNT;
    SAMPLE_DELAY_MS = config["sampleDelayMs"] | SAMPLE_DELAY_MS;
    PATH_LOSS_EXPONENT = config["pathLossExponent"] | PATH_LOSS_EXPONENT;
    
    if (!config["kalman"].isNull()) {
      kalman_P = config["kalman"]["P"] | kalman_P;
      kalman_Q = config["kalman"]["Q"] | kalman_Q;
      kalman_R = config["kalman"]["R"] | kalman_R;
      kalman_initial = config["kalman"]["initial"] | kalman_initial;
      rssiFilter.updateParameters(kalman_Q, kalman_R, kalman_P, kalman_initial);
    }
  }
  
  // Load explicit targets if needed
  if (DISCOVERY_MODE == "explicit") {
    JsonArray targets = doc["targets"];
    for (JsonVariant target : targets) {
      deviceStatuses.push_back({target.as<String>()});
    }
  }
  
  lastConfigVersion = version;
  Serial.println("[CONFIG] Updated v" + String(version) + " Mode:" + DISCOVERY_MODE);
  return true;
}

// Initialize WiFi connection
void connectWiFi() {
  Serial.println("[WIFI] Connecting...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts++ < 20) delay(250);

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WIFI] Connected: " + WiFi.localIP().toString());
  } else {
    Serial.println("[WIFI] Failed to connect!");
  }
}

// Calibration mode scanning
void calibrationScan() {
  static float rssiSum = 0;
  static int count = 0;
  
  BLEScanResults* results = pBLEScan->start(1, false);
  
  for (int i = 0; i < results->getCount(); i++) {
    BLEAdvertisedDevice device = results->getDevice(i);
    if (device.getName() == TARGET_DEVICE_NAME) {
      float rssi = device.getRSSI();
      rssiSum += rssi;
      count++;
      
      if (count % 10 == 0) {
        float avg = rssiSum / count;
        Serial.printf("\n[CAL] Samples:%d AvgRSSI@1m:%.1f Distance:%.2fm\n", 
                      count, avg, calculateDistance(avg));
        Serial.println("[CAL] Set TX_POWER = " + String(avg, 0));
      }
    }
  }
  
  pBLEScan->clearResults();
  delay(500);
}

void setup() {
  Serial.begin(115200);
  Serial.printf("\n[SYSTEM] %s v2.0\n", DEVICE_NAME);
  
  if (CALIBRATION_MODE) {
    Serial.println("\n===== CALIBRATION MODE =====");
    Serial.println("Place tag at 1 meter distance");
    Serial.println("TX_POWER: " + String(TX_POWER));
    deviceStatuses.push_back({TARGET_DEVICE_NAME});
  } else {
    connectWiFi();
    
    if (WiFi.status() == WL_CONNECTED) {
      configFetched = fetchConfig();
    }
    
    if (deviceStatuses.empty()) {
      deviceStatuses.push_back({TARGET_DEVICE_NAME});
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
  static unsigned long lastConfigCheck = 0;
  static unsigned long lastNetworkCheck = 0;
  
  if (CALIBRATION_MODE) {
    calibrationScan();
    return;
  }
  
  // Network maintenance
  if (millis() - lastNetworkCheck > NETWORK_CHECK_INTERVAL) {
    if (WiFi.status() != WL_CONNECTED) connectWiFi();
    lastNetworkCheck = millis();
  }
  
  // Configuration updates
  if (millis() - lastConfigCheck > VERSION_CHECK_INTERVAL) {
    if (WiFi.status() == WL_CONNECTED) fetchConfig();
    lastConfigCheck = millis();
  }
  
  // Main scanning
  scanAllDevices();
  
  // Adaptive delay
  bool present = false;
  for (auto& d : deviceStatuses) {
    if (d.isPresent) { present = true; break; }
  }
  delay(present ? 1000 : 2000);
}