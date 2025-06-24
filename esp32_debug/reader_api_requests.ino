#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// === Configuration ===
const char* ssid = "POCO X6 5G";
const char* password = "12345678";
const char* serverUrl = "https://150.230.8.22/api";  // Changed to HTTPS
const char* readerKey = "ESP32_READER_KEY";
#define DEVICE_NAME "Asset_Reader_01"

// Test endpoints
const char* endpoint1 = "/reader-config";  // Will use with query param
const char* endpoint2 = "/reader-log";     // For testing POST

// Timing
unsigned long requestCount = 0;
unsigned long successCount = 0;
unsigned long failCount = 0;
unsigned long lastHeapReport = 0;

// Use WiFiClientSecure for HTTPS
WiFiClientSecure wifiClient;

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
    
    // Configure SSL/TLS
    // For testing with self-signed certificates or IP addresses
    wifiClient.setInsecure(); // Skip certificate verification
    Serial.println("[SSL] Certificate verification disabled (for testing)");
  } else {
    Serial.println("\n[WIFI] Failed to connect!");
  }
}

void testGetRequest() {
  requestCount++;
  Serial.println("\n--- GET Request #" + String(requestCount) + " ---");
  Serial.println("[HEAP] Free: " + String(ESP.getFreeHeap()) + " bytes");
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[ERROR] WiFi not connected");
    failCount++;
    return;
  }
  
  // Build URL with query parameter
  String url = String(serverUrl) + endpoint1 + "?reader_name=" + DEVICE_NAME;
  Serial.println("[HTTP] GET " + url);
  
  // Create new HTTPClient instance
  HTTPClient https;
  
  // Use WiFiClientSecure for HTTPS
  if (!https.begin(wifiClient, url)) {
    Serial.println("[HTTPS] Failed to begin connection");
    failCount++;
    return;
  }
  
  https.addHeader("X-Reader-Key", readerKey);
  https.setTimeout(10000); // Increased timeout for HTTPS
  
  unsigned long startTime = millis();
  int httpCode = https.GET();
  unsigned long duration = millis() - startTime;
  
  Serial.println("[HTTP] Response Code: " + String(httpCode));
  Serial.println("[HTTP] Duration: " + String(duration) + "ms");
  
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK || httpCode == 200) {
      String payload = https.getString();
      Serial.println("[HTTP] Response length: " + String(payload.length()) + " bytes");
      
      // Try to parse JSON to verify it's valid
      DynamicJsonDocument doc(2048); // Increased size
      DeserializationError error = deserializeJson(doc, payload);
      
      if (error) {
        Serial.println("[JSON] Parse error: " + String(error.c_str()));
        // Print first 200 chars of response for debugging
        Serial.println("[DEBUG] Response preview: " + payload.substring(0, 200));
      } else {
        Serial.println("[JSON] Parse success!");
        // Print some fields if they exist
        if (doc.containsKey("name")) {
          Serial.println("[JSON] Reader name: " + String(doc["name"].as<const char*>()));
        }
        if (doc.containsKey("version")) {
          Serial.println("[JSON] Version: " + String(doc["version"].as<unsigned long>()));
        }
        if (doc.containsKey("discovery_mode")) {
          Serial.println("[JSON] Discovery mode: " + String(doc["discovery_mode"].as<const char*>()));
        }
      }
      successCount++;
    } else {
      String errorResponse = https.getString();
      Serial.println("[HTTP] Error response (" + String(httpCode) + "): " + errorResponse.substring(0, 200));
      failCount++;
    }
  } else {
    Serial.println("[HTTP] Error: " + https.errorToString(httpCode));
    Serial.println("[HTTP] Error code: " + String(httpCode));
    failCount++;
  }
  
  https.end();
  Serial.println("[HEAP] After request: " + String(ESP.getFreeHeap()) + " bytes");
}

void testPostRequest() {
  requestCount++;
  Serial.println("\n--- POST Request #" + String(requestCount) + " ---");
  Serial.println("[HEAP] Free: " + String(ESP.getFreeHeap()) + " bytes");
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[ERROR] WiFi not connected");
    failCount++;
    return;
  }
  
  String url = String(serverUrl) + endpoint2;
  Serial.println("[HTTP] POST " + url);
  
  // Create test data
  StaticJsonDocument<256> doc;
  doc["reader_name"] = DEVICE_NAME;
  doc["device_name"] = "Asset_Test_01";
  doc["type"] = "heartbeat";
  doc["status"] = "present";
  doc["estimated_distance"] = 2.5;
  doc["rssi"] = -65;
  doc["kalman_rssi"] = -64.5;
  
  String jsonString;
  serializeJson(doc, jsonString);
  Serial.println("[JSON] Payload: " + jsonString);
  
  // Create new HTTPClient instance
  HTTPClient https;
  
  if (!https.begin(wifiClient, url)) {
    Serial.println("[HTTPS] Failed to begin connection");
    failCount++;
    return;
  }
  
  https.addHeader("Content-Type", "application/json");
  https.addHeader("X-Reader-Key", readerKey);
  https.setTimeout(10000); // Increased timeout for HTTPS
  
  unsigned long startTime = millis();
  int httpCode = https.POST(jsonString);
  unsigned long duration = millis() - startTime;
  
  Serial.println("[HTTP] Response Code: " + String(httpCode));
  Serial.println("[HTTP] Duration: " + String(duration) + "ms");
  
  if (httpCode > 0) {
    String response = https.getString();
    Serial.println("[HTTP] Response: " + response);
    
    if (httpCode >= 200 && httpCode < 300) {
      successCount++;
    } else {
      failCount++;
    }
  } else {
    Serial.println("[HTTP] Error: " + https.errorToString(httpCode));
    Serial.println("[HTTP] Error code: " + String(httpCode));
    failCount++;
  }
  
  https.end();
  Serial.println("[HEAP] After request: " + String(ESP.getFreeHeap()) + " bytes");
}

void printStats() {
  Serial.println("\n========== STATISTICS ==========");
  Serial.println("Total Requests: " + String(requestCount));
  Serial.println("Successful: " + String(successCount));
  Serial.println("Failed: " + String(failCount));
  float successRate = (requestCount > 0) ? (float(successCount) / float(requestCount) * 100.0) : 0;
  Serial.println("Success Rate: " + String(successRate, 1) + "%");
  Serial.println("Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
  Serial.println("Largest Free Block: " + String(ESP.getMaxAllocHeap()) + " bytes");
  Serial.println("Uptime: " + String(millis() / 1000) + " seconds");
  Serial.println("WiFi RSSI: " + String(WiFi.RSSI()) + " dBm");
  Serial.println("================================\n");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=== ESP32 API Debug Test (HTTPS) ===");
  Serial.println("Device: " + String(DEVICE_NAME));
  Serial.println("Server: " + String(serverUrl));
  Serial.println("Initial Heap: " + String(ESP.getFreeHeap()) + " bytes");
  
  connectWiFi();
  
  // Test initial connection
  Serial.println("\nTesting HTTPS connection...");
  testGetRequest();
  
  Serial.println("\nStarting API test loop in 3 seconds...");
  delay(3000);
}

void loop() {
  // Check WiFi and reconnect if needed
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\n[WIFI] Connection lost, reconnecting...");
    connectWiFi();
    delay(2000);
  }
  
  // Alternate between GET and POST requests
  if (requestCount % 2 == 0) {
    testGetRequest();
  } else {
    testPostRequest();
  }
  
  // Print statistics every 10 requests
  if (requestCount % 10 == 0) {
    printStats();
  }
  
  // Monitor heap every 30 seconds
  if (millis() - lastHeapReport > 30000) {
    Serial.println("\n[HEAP MONITOR] Free: " + String(ESP.getFreeHeap()) + 
                   ", Largest block: " + String(ESP.getMaxAllocHeap()));
    lastHeapReport = millis();
  }
  
  // Optional: Force garbage collection
  if (requestCount % 20 == 0) {
    Serial.println("[SYSTEM] Running cleanup...");
    delay(100);
  }
  
  // Delay between requests
  delay(5000);
}