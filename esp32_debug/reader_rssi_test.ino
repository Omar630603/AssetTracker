#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <map>

#define DEVICE_NAME "Asset_RSSI_Test_01"
const char* tagNames[] = {"Asset_Tag_01", "Asset_Tag_02"};
const int numTags = 2;
const int NUM_CYCLES = 10;   // 10 cycles per test
const int SCAN_TIME = 3;     // seconds per BLE scan

class KalmanFilter {
private:
  float Q, R, P, X, K;
  bool initialized;
public:
  KalmanFilter(float q=0.1, float r=2.0, float p=1.0, float init=-60.0)
    : Q(q), R(r), P(p), X(init), K(0), initialized(false) {}
  float update(float measurement) {
    if (!initialized) {
      X = measurement;
      initialized = true;
      return X;
    }
    P = P + Q;
    K = P / (P + R);
    X = X + K * (measurement - X);
    P = (1 - K) * P;
    return X;
  }
  void reset(float init=-60.0) {
    initialized = false; P = 1.0; X = init; K = 0;
  }
};
std::map<String, KalmanFilter> kalmanFilters;

bool isTargetTag(const String& name) {
  for (int i = 0; i < numTags; i++)
    if (name == tagNames[i]) return true;
  return false;
}

float calculateDistance(float rssi, float txPower=-68.0, float n=2.5) {
  if (rssi == 0 || rssi < -100) return -1.0;
  float distance = pow(10, (txPower - rssi) / (10.0 * n));
  return constrain(distance, 0.01, 100.0);
}

// Read a float from serial
float getFloatFromSerial(const char* prompt) {
  float value = 0;
  bool gotValue = false;
  while (!gotValue) {
    Serial.print(prompt);
    while (!Serial.available()) delay(50);
    String input = Serial.readStringUntil('\n');
    value = input.toFloat();
    if (value > 0) {
      gotValue = true;
    } else {
      Serial.println("Invalid value, try again.");
    }
  }
  return value;
}

// Read an int from serial
int getIntFromSerial(const char* prompt) {
  int value = 0;
  bool gotValue = false;
  while (!gotValue) {
    Serial.print(prompt);
    while (!Serial.available()) delay(50);
    String input = Serial.readStringUntil('\n');
    value = input.toInt();
    if (value > 0) {
      gotValue = true;
    } else {
      Serial.println("Invalid value, try again.");
    }
  }
  return value;
}

void runScanCycle(int cycleNum, int trial, float refDist) {
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
  pBLEScan->setActiveScan(true);

  std::map<String, float> rssiResults;
  for (int i = 0; i < numTags; i++) rssiResults[tagNames[i]] = -100.0;

  BLEScanResults* foundDevices = pBLEScan->start(SCAN_TIME, false);
  int deviceCount = foundDevices->getCount();

  for (int i = 0; i < deviceCount; i++) {
    BLEAdvertisedDevice device = foundDevices->getDevice(i);
    if (device.haveName()) {
      String name = device.getName().c_str();
      if (isTargetTag(name)) {
        float rssi = device.getRSSI();
        if (rssi > rssiResults[name]) rssiResults[name] = rssi;
      }
    }
  }
  pBLEScan->clearResults();

  for (int i = 0; i < numTags; i++) {
    String name = tagNames[i];
    float rawRssi = rssiResults[name];
    float filteredRssi = kalmanFilters[name].update(rawRssi);
    float distance = calculateDistance(filteredRssi);

    Serial.printf("%d,%.2f,%d,%s,%.1f,%.1f,%.2f\n",
                  trial, refDist, cycleNum+1, name.c_str(), rawRssi, filteredRssi, distance);
  }
}

void waitForEnter() {
  Serial.println("\n[PAUSED] --- Press ENTER in Serial Monitor to run another test set ---");
  while (!Serial.available()) delay(100);
  while (Serial.available()) Serial.read();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== ESP32 BLE RSSI/Kalman Test (CSV Logging/Input Version) ===");
  Serial.printf("Target tags: %s, %s\n", tagNames[0], tagNames[1]);
  Serial.printf("Num cycles: %d, Scan time: %ds\n", NUM_CYCLES, SCAN_TIME);

  esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
  BLEDevice::init(DEVICE_NAME);

  for (int i = 0; i < numTags; i++)
    kalmanFilters[tagNames[i]].reset(-60.0);

  delay(1000);
}

void loop() {
  float refDistance = getFloatFromSerial("Enter reference distance (m): ");
  int trial = getIntFromSerial("Enter trial number: ");
  Serial.printf("Using reference distance: %.2f m, trial number: %d\n", refDistance, trial);

  Serial.println("\ntrial,ref_distance,cycle,tag,raw_rssi,kalman_rssi,estimated_distance");
  Serial.println("====== NEW TEST SET START ======\n");

  for (int i = 0; i < numTags; i++)
    kalmanFilters[tagNames[i]].reset(-60.0);

  for (int cycle = 0; cycle < NUM_CYCLES; cycle++) {
    runScanCycle(cycle, trial, refDistance);
    delay(1000);
  }

  Serial.println("\n====== TEST SET COMPLETE ======");
  waitForEnter();
}
