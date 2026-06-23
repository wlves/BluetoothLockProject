#include <BLEDevice.h>

#define LED_PIN      2
#define TARGET_NAME  "vivo V29"
#define RSSI_ON      -65   
#define RSSI_OFF     -80    
#define SMOOTH_COUNT  5   

BLEScan* scan;

int rssiBuffer[SMOOTH_COUNT];
int bufIndex  = 0;
bool bufFull  = false;

void addRSSI(int rssi) {
  rssiBuffer[bufIndex] = rssi;
  bufIndex = (bufIndex + 1) % SMOOTH_COUNT;
  if (bufIndex == 0) bufFull = true;
}

int smoothRSSI() {
  int count = bufFull ? SMOOTH_COUNT : bufIndex;
  if (count == 0) return -100;
  int sum = 0;
  for (int i = 0; i < count; i++) sum += rssiBuffer[i];
  return sum / count;
}

bool ledState = false;
void setLED(bool on) {
  if (on == ledState) return;
  ledState = on;
  digitalWrite(LED_PIN, on ? HIGH : LOW);
  Serial.println(on ? "LED ON (Near)" : "LED OFF (Far)");
}

// ── Setup ─────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  BLEDevice::init("ESP32_RSSI_LED");
  scan = BLEDevice::getScan();
  scan->setActiveScan(true);
  scan->setInterval(100);
  scan->setWindow(80);

  Serial.println("Scanning for: " + String(TARGET_NAME));
}

void loop() {
  BLEScanResults* results = scan->start(1, false);
  bool found = false;

  for (int i = 0; i < results->getCount(); i++) {
    BLEAdvertisedDevice device = results->getDevice(i);

    if (!device.haveName()) continue;
    String name = device.getName().c_str();
    if (name != TARGET_NAME) continue;

    int rssi = device.getRSSI();
    addRSSI(rssi);
    int avg = smoothRSSI();
    found = true;

    Serial.printf("RAW: %d dBm | AVG: %d dBm\n", rssi, avg);

    if      (avg >= RSSI_ON)  setLED(true);
    else if (avg <= RSSI_OFF) setLED(false);

    break;
  }

  if (!found) {
    Serial.println("Target not found → LED OFF");
    addRSSI(-100);
    setLED(false);
  }

  scan->clearResults();
  delay(300);
}