#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>

// === Configuration ===
#define TARGET_DEVICE_NAME "Asset_Tag_01"  // BLE tag name
#define DEVICE_NAME "Asset_Reader_01"      // Reader name
#define TX_POWER -52                       // Calibrated RSSI at 1 meter (adjust after field test)
#define PATH_LOSS_EXPONENT 2.5             // Typical indoor; tune based on environment
#define MAX_DISTANCE 5.0                   // Max allowed distance in meters
#define SAMPLE_COUNT 10                    // Number of RSSI samples per batch

BLEScan* pBLEScan;

// === Kalman Filter Class ===
class KalmanFilter {
public:
  KalmanFilter(float processNoise, float measurementNoise, float estimatedError, float initialValue) {
    Q = processNoise;
    R = measurementNoise;
    P = estimatedError;
    X = initialValue;
  }

  float update(float measurement) {
    // Prediction update
    P = P + Q;

    // Measurement update
    K = P / (P + R);
    X = X + K * (measurement - X);
    P = (1 - K) * P;

    return X;
  }

private:
  float Q;  // Process noise covariance
  float R;  // Measurement noise covariance
  float P;  // Estimation error covariance
  float K;  // Kalman gain
  float X;  // Value
};

// === Initialize Kalman Filter ===
KalmanFilter rssiFilter(0.01, 1, 1, -50);  // Adjust Q, R as needed

// === Utility Function ===
float calculateDistance(float rssi) {
  if (rssi == 0) return -1.0;  // Avoid invalid calculations

  float ratio_db = TX_POWER - rssi;
  float distance = pow(10, ratio_db / (10 * PATH_LOSS_EXPONENT));

  // Apply piecewise correction for close range (<2 m)
  if (distance < 2.0) {
    distance *= 0.5;  // Example: halve distance for near-field correction
  }

  return distance;
}

void setup() {
  Serial.begin(9600);
  Serial.println("Starting BLE continuous scanner...");

  BLEDevice::init(DEVICE_NAME);

  pBLEScan = BLEDevice::getScan();
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
  pBLEScan->setActiveScan(true);
}

void loop() {
  int sampleCounter = 0;
  float rssiSum = 0;

  while (sampleCounter < SAMPLE_COUNT) {
    BLEScanResults* foundDevices = pBLEScan->start(1, false);  // 1 second scan

    for (int i = 0; i < foundDevices->getCount(); i++) {
      BLEAdvertisedDevice device = foundDevices->getDevice(i);

      if (device.getName() == TARGET_DEVICE_NAME) {
        int rssi = device.getRSSI();
        rssiSum += rssi;
        sampleCounter++;

        Serial.print("Sample ");
        Serial.print(sampleCounter);
        Serial.print(": Raw RSSI = ");
        Serial.println(rssi);

        if (sampleCounter >= SAMPLE_COUNT) {
          break;
        }
      }
    }

    pBLEScan->clearResults();
    delay(100);  // Small delay between scans
  }

  // === After collecting samples ===
  float rssiAvg = rssiSum / SAMPLE_COUNT;
  float filteredRssi = rssiFilter.update(rssiAvg);

  float rawDistance = calculateDistance(rssiAvg);
  float filteredDistance = calculateDistance(filteredRssi);

  Serial.print("Avg RSSI = ");
  Serial.print(rssiAvg);
  Serial.print(" dBm | Filtered RSSI = ");
  Serial.print(filteredRssi);
  Serial.print(" dBm | Raw Distance = ");
  Serial.print(rawDistance, 2);
  Serial.print(" m | Filtered Distance = ");
  Serial.print(filteredDistance, 2);
  Serial.println(" m");

  // Check distance threshold
  if (filteredDistance > MAX_DISTANCE) {
    Serial.println("⚠ ALERT: Asset has likely left the room (distance > 5m)");
    // TODO: Send alert signal to server here
  } else {
    Serial.println("✅ Asset is within allowed range.");
    // TODO: Send heartbeat/status signal to server here
  }

  // Reset for next round
  sampleCounter = 0;
  rssiSum = 0;

  delay(500);  // Half a second pause before next round
}
