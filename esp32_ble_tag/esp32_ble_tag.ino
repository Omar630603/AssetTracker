// ========================
// BLE TAG (ESP32 TAG DEVICE)
// ========================
#include <BLEDevice.h>

#define TAG_NAME "Asset_Tag_01"

void setup() {
  Serial.begin(9600);
  BLEDevice::init(TAG_NAME);

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x06);  // Helps iOS discovery
  pAdvertising->setMinPreferred(0x12);  // Helps iOS discovery

  BLEAdvertisementData advertisementData;
  advertisementData.setName(TAG_NAME);
  pAdvertising->setAdvertisementData(advertisementData);

  BLEDevice::startAdvertising();
  Serial.println("BLE Tag advertising started...");
}

void loop() {
  // Nothing needed here; the BLE stack handles advertising.
  delay(1000);
}