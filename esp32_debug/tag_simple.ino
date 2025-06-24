#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>

// ========================
// BLE TAG CONFIGURATION
// ========================
#define TAG_NAME "Asset_Tag_01"  // Change this for each tag
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// Global Variables
BLEServer* pServer = nullptr;
BLECharacteristic* pCharacteristic = nullptr;
bool deviceConnected = false;
unsigned long bootTime;
unsigned long lastHeartbeat = 0;

// Connection callbacks
class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("[BLE] Device connected");
  }
  
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("[BLE] Device disconnected");
    delay(500);
    // Restart advertising
    pServer->startAdvertising();
    Serial.println("[BLE] Advertising restarted");
  }
};

void setup() {
  Serial.begin(115200);
  bootTime = millis();
  
  Serial.printf("\n=== BLE Asset Tag ===\n");
  Serial.printf("Tag Name: %s\n", TAG_NAME);
  Serial.printf("Initial heap: %d bytes\n", ESP.getFreeHeap());
  
  // Initialize BLE
  Serial.println("\n[BLE] Initializing...");
  BLEDevice::init(TAG_NAME);
  
  // Set TX Power for better range
  BLEDevice::setPower(ESP_PWR_LVL_P7); // +7 dBm (maximum)
  
  // Create BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  
  // Create Service
  BLEService* pService = pServer->createService(SERVICE_UUID);
  
  // Create Characteristic
  pCharacteristic = pService->createCharacteristic(
    CHAR_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_NOTIFY
  );
  
  // Add descriptor
  pCharacteristic->addDescriptor(new BLE2902());
  
  // Set initial value
  pCharacteristic->setValue(TAG_NAME);
  
  // Start service
  pService->start();
  
  // Configure advertising
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // 7.5ms
  pAdvertising->setMaxPreferred(0x12);  // 22.5ms
  
  // Start advertising
  BLEDevice::startAdvertising();
  
  Serial.println("[BLE] Service started");
  Serial.println("[BLE] Advertising started");
  Serial.printf("[BLE] Service UUID: %s\n", SERVICE_UUID);
  Serial.printf("[BLE] TX Power: +7 dBm\n");
  Serial.println("[SYSTEM] Tag ready!\n");
}

void loop() {
  // Heartbeat every 30 seconds
  if (millis() - lastHeartbeat > 30000) {
    float uptime = (millis() - bootTime) / 1000.0 / 60.0;
    
    Serial.printf("[HEARTBEAT] Uptime: %.1f min | Connected: %s | Heap: %d bytes\n",
                  uptime,
                  deviceConnected ? "Yes" : "No",
                  ESP.getFreeHeap());
    
    // Update characteristic with status
    if (deviceConnected) {
      String status = String(TAG_NAME) + " | Up: " + String(uptime, 1) + "m";
      pCharacteristic->setValue(status.c_str());
      pCharacteristic->notify();
    }
    
    lastHeartbeat = millis();
  }
  
  delay(1000);
}