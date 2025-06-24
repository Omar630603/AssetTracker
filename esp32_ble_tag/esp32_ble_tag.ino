#include <BLEDevice.h>

// ========================
// BLE TAG
// ========================
#define TAG_NAME "Asset_Tag_01" 
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b" 
#define MANUFACTURER_DATA "esp_asset_tracker"

// Configuration
const int TX_POWER = ESP_PWR_LVL_P7;  // +7 dBm for maximum range

// Statistics
unsigned long bootTime;
unsigned long lastStatusPrint = 0;

void setup() {
  Serial.begin(115200);
  bootTime = millis();

  Serial.println("\n=== BLE Tag ===");
  Serial.printf("Tag Name: %s\n", TAG_NAME);
  Serial.printf("Initial heap: %d bytes\n", ESP.getFreeHeap());

  // Initialize BLE (leave name empty for stealth)
  BLEDevice::init(TAG_NAME);

  // Set TX Power for maximum range
  BLEDevice::setPower((esp_power_level_t)TX_POWER);
  Serial.printf("[BLE] TX Power: +%d dBm\n", TX_POWER);

  // Set up custom advertising
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->setAdvertisementType(ADV_TYPE_NONCONN_IND); // Non-connectable advertising

  BLEAdvertisementData advData;
  // Add device name
  advData.setName(TAG_NAME);
  // Add the shared service UUID
  advData.setCompleteServices(BLEUUID(SERVICE_UUID));
  // Add manufacturer data from the define
  advData.setManufacturerData(MANUFACTURER_DATA);

  pAdvertising->setAdvertisementData(advData);

  pAdvertising->start();

  Serial.println("[BLE] Advertising started");
  Serial.println("[SYSTEM] Tag ready!\n");
}

void loop() {
  // Print status every 30 seconds just to show the tag is alive
  if (millis() - lastStatusPrint > 30000) {
    float uptime = (millis() - bootTime) / 1000.0 / 60.0;

    Serial.printf("[STATUS] Uptime: %.1f min | Heap: %d bytes | Still advertising...\n",
                  uptime,
                  ESP.getFreeHeap());

    lastStatusPrint = millis();
  }

  delay(1000);
}