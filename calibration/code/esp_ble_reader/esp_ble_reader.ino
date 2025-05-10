#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>

#define SCAN_INTERVAL 1000  // in milliseconds
#define TARGET_DEVICE_NAME "Asset_Tag_01"  // The BLE name of the tag
#define DEVICE_NAME "Asset_Reader_01"  // The BLE name of the reader
#define TX_POWER -47  // Adjust based on RSSI at 1 meter

BLEScan* pBLEScan;

// Function to calculate distance from RSSI
float calculateDistance(int rssi) {
  return pow(10, (TX_POWER - rssi) / 20.0);
}

void setup() {
  Serial.begin(9600);
  Serial.println("Starting BLE Scanner...");

  // Initialize BLE
  BLEDevice::init(DEVICE_NAME);

  // Set up BLE scanner
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
  pBLEScan->setActiveScan(true);  // Enable active scanning to get RSSI values
}

void loop() {
  // Wait for input from Serial Monitor to start 10 readings
  Serial.println("Type 'r' and press Enter to start a new set of 10 readings...");
  while (Serial.available() == 0 || Serial.read() != 'r') {
    delay(100);  // Wait until 'r' is entered
  }

  Serial.println("Starting 10 readings...");
  int readingCount = 0;

  while (readingCount < 10) {
    Serial.println("Scanning...");

    // Scan for devices
    BLEScanResults* foundDevices = pBLEScan->start(SCAN_INTERVAL / 1000);

    for (int i = 0; i < foundDevices->getCount(); i++) {
      BLEAdvertisedDevice device = foundDevices->getDevice(i);

      // Check if this is the target device
      if (device.getName() == TARGET_DEVICE_NAME) {
        int rssi = device.getRSSI();
        float distance = calculateDistance(rssi);

        Serial.print("Reading ");
        Serial.print(readingCount + 1);
        Serial.print(": RSSI = ");
        Serial.print(rssi);
        Serial.print(" | Calculated Distance = ");
        Serial.print(distance);
        Serial.println(" meters");

        readingCount++;
        delay(500);  // Small delay between readings

        // Stop scanning if we've reached 10 readings
        if (readingCount >= 10) {
          Serial.println("Completed 10 readings.");
          break;
        }
      }
    }

    pBLEScan->clearResults();
    delay(SCAN_INTERVAL);  // Wait before the next scan
  }

  // End of 10 readings, prompt user for next action
  Serial.println("Type 'r' and press Enter to start a new set of 10 readings...");
}
