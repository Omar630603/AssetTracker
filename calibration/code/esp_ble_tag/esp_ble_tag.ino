#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define DEVICE_NAME "Asset_Tag_01" 

void setup() {
  Serial.begin(9600);
  BLEDevice::init(DEVICE_NAME);
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(BLEUUID((uint16_t)0x180D));
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinInterval(1000); 
  pAdvertising->setMaxInterval(1000);
  BLEDevice::startAdvertising();

  Serial.println("BLE Tag is now broadcasting...");
}

void loop() {
  // 
}
