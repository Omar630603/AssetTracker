// ========================
// BLE TAG (ESP32 TAG DEVICE)
// ========================
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <mbedtls/aes.h>
#include <Preferences.h>

// Tag Configuration
#define TAG_NAME_PREFIX "Asset_"
#define TAG_ID "Tag_01"
#define COMPANY_ID 0xFFFF
#define SECRET_KEY "ESP32_TAG_SECRET_KEY"
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// Global State
Preferences prefs;
String fullTagName;
uint8_t authToken[16];
BLECharacteristic* pChar;
unsigned long bootTime;

// Manufacturer data structure for BLE advertisement
struct MfgData {
  uint16_t companyId;
  uint8_t dataType;    // 0x01 = Asset Tag
  uint8_t version;     // Protocol version
  uint8_t authToken[4]; // First 4 bytes of auth
} __attribute__((packed));

// Generate authentication token using AES
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

// BLE connection callbacks
class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    Serial.println("[BLE] Device connected");
  }
  
  void onDisconnect(BLEServer* pServer) {
    Serial.println("[BLE] Device disconnected");
    // Restart advertising
    delay(500);
    pServer->startAdvertising();
  }
};

String bytesToHex(uint8_t* data, size_t len) {
  String s = "";
  for (size_t i = 0; i < len; ++i) {
    if (data[i] < 16) s += "0";
    s += String(data[i], HEX);
  }
  return s;
}

void setup() {
  Serial.begin(115200);
  bootTime = millis();
  
  // Initialize persistent storage
  prefs.begin("asset-tag", false);
  
  // Generate tag name
  fullTagName = String(TAG_NAME_PREFIX) + String(TAG_ID);
  
  // Generate or load unique device ID
  if (!prefs.isKey("deviceId")) {
    prefs.putUInt("deviceId", esp_random());
  }
  uint32_t deviceId = prefs.getUInt("deviceId");
  
  // Generate authentication token
  generateAuthToken(fullTagName, authToken);
  
  Serial.printf("\n[TAG] %s\n", fullTagName.c_str());
  Serial.printf("[TAG] Device ID: %08X\n", deviceId);
  Serial.print("[TAG] Auth Token: ");
  for (int i = 0; i < 4; i++) Serial.printf("%02X ", authToken[i]);
  Serial.println("\n");
  
  // Initialize BLE
  BLEDevice::init(fullTagName.c_str());
  BLEDevice::setPower(ESP_PWR_LVL_N0); // 0 dBm transmit power
  
  // Create BLE Server and Service
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  
  BLEService* pService = pServer->createService(SERVICE_UUID);
  
  // Create characteristic with full auth token
  pChar = pService->createCharacteristic(
    CHAR_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_NOTIFY
  );
  
  pChar->addDescriptor(new BLE2902());
  pChar->setValue(authToken, 16);
  
  // Start service
  pService->start();
  
  // Configure advertising
  BLEAdvertising* pAdv = BLEDevice::getAdvertising();
  
  // Add service UUID
  pAdv->addServiceUUID(SERVICE_UUID);
  
  // Create manufacturer data
  MfgData mfg = {
    .companyId = COMPANY_ID,
    .dataType = 0x01,
    .version = 0x01
  };
  memcpy(mfg.authToken, authToken, 4);
  
  // Set advertisement data
  BLEAdvertisementData advData;
  advData.setName(fullTagName.c_str());
  advData.setManufacturerData(bytesToHex((uint8_t*)&mfg, sizeof(mfg)));
  pAdv->setAdvertisementData(advData);
  
  // Set scan response
  BLEAdvertisementData scanData;
  scanData.setCompleteServices(BLEUUID(SERVICE_UUID));
  pAdv->setScanResponseData(scanData);
  
  // Configure advertising parameters
  pAdv->setScanResponse(true);
  pAdv->setMinPreferred(0x06); // 7.5ms
  pAdv->setMaxPreferred(0x12); // 22.5ms
  
  // Start advertising
  BLEDevice::startAdvertising();
  
  Serial.println("[TAG] Advertising started");
  Serial.println("[TAG] Service UUID: " + String(SERVICE_UUID));
}

void loop() {
  static unsigned long lastRotation = 0;
  static unsigned long lastStatus = 0;
  const unsigned long ROTATION_INTERVAL = 3600000; // 1 hour
  const unsigned long STATUS_INTERVAL = 30000;     // 30 seconds
  
  unsigned long now = millis();
  
  // Rotate authentication token periodically
  if (now - lastRotation > ROTATION_INTERVAL) {
    // Generate new token with timestamp
    generateAuthToken(fullTagName + String(now), authToken);
    
    // Update characteristic value
    pChar->setValue(authToken, 16);
    pChar->notify();
    
    // Update would require restarting advertising with new mfg data
    // For simplicity, keeping advertisement token static
    
    lastRotation = now;
    Serial.println("[TAG] Token rotated");
  }
  
  // Print status periodically
  if (now - lastStatus > STATUS_INTERVAL) {
    float uptime = (now - bootTime) / 1000.0 / 60.0;
    Serial.printf("[TAG] Status: Active, Uptime: %.1f min\n", uptime);
    lastStatus = now;
  }
  
  // Low power delay
  delay(1000);
  
  // Optional: Add deep sleep between advertisements for battery saving
  // esp_sleep_enable_timer_wakeup(1000000); // 1 second
  // esp_light_sleep_start();
}