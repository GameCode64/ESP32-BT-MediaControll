#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEHIDDevice.h>
#include <HIDTypes.h>
#include <HIDKeyboardTypes.h>

#define BT_SYNC 13
#define BTN_ARRAY 4

bool BTN_A = false;
bool BTN_B = false;
bool BTN_C = false;
bool BTN_D = false;

BLEHIDDevice* HID;
BLECharacteristic* BT_Input;
BLECharacteristic* BT_Output;

void setup() {
  pinMode(BT_SYNC, INPUT_PULLUP);
  Serial.begin(115200);

  BLEDevice::init("GC64RC");

  BLEServer* server = BLEDevice::createServer();

  HID = new BLEHIDDevice(server);
  BT_Input = HID->inputReport(1);    // Report ID 1
  BT_Output = HID->outputReport(1);  // Report ID 1

  HID->manufacturer()->setValue("GC64");
  HID->pnp(0x01, 0x02e5, 0xabcd, 0x0110);
  HID->hidInfo(0x00, 0x01);

  BLESecurity* security = new BLESecurity();
  security->setAuthenticationMode(ESP_LE_AUTH_BOND);

  const uint8_t reportMap[] = {
    0x05, 0x0C,        // Usage Page (Consumer Devices)
    0x09, 0x01,        // Usage (Consumer Control)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        // Report ID (1)
    0x19, 0x00,        // Usage Minimum (0)
    0x2A, 0x3C, 0x02,  // Usage Maximum (0x23C)
    0x15, 0x00,        // Logical Minimum (0)
    0x26, 0x3C, 0x02,  // Logical Maximum (0x23C)
    0x95, 0x01,        // Report Count (1)
    0x75, 0x10,        // Report Size (16)
    0x81, 0x00,        // Input (Data, Array)
    0xC0               // End Collection
  };

  HID->reportMap((uint8_t*)reportMap, sizeof(reportMap));
  HID->startServices();

  BLEAdvertising* advertising = server->getAdvertising();
  advertising->setAppearance(HID_KEYBOARD);
  advertising->addServiceUUID(HID->hidService()->getUUID());
  advertising->start();
  HID->setBatteryLevel(100);

  // Check if the sync pin is grounded at startup
  if (digitalRead(BT_SYNC) == LOW) {
    SyncBT();
  }
}


void SyncBT() {
  Serial.println("Entering sync mode...");
  BLEDevice::startAdvertising();
  // Additional sync mode logic here
}


void sendMediaKey(uint16_t key) {
  uint8_t mediaKey[2];
  mediaKey[0] = key & 0xFF;
  mediaKey[1] = (key >> 8) & 0xFF;

  BT_Input->setValue(mediaKey, 2);
  BT_Input->notify();

  // Release the key
  mediaKey[0] = 0x00;
  mediaKey[1] = 0x00;
  BT_Input->setValue(mediaKey, 2);
  BT_Input->notify();
}


void BtnQueue() {
  int Sens = analogRead(BTN_ARRAY);
  if (Sens > 3700)  // BTN A
  {
    BTN_A = true;
    sendMediaKey(0xE9); // Vol+
  } else if (Sens > 2500 && Sens <= 3700)  // BTN B
  {
    BTN_B = true;
    sendMediaKey(0xEA); // Vol-
  } else if (Sens > 1500 && Sens <= 2500)  // BTN C
  {
    BTN_C = true;
    sendMediaKey(0xE2); // Mute
  } else if (Sens > 240 && Sens <= 1500)  // BTN D
  {
    sendMediaKey(0xCD); // Play/Pause
    BTN_D = true;
  }
}

void BtnHandler() {
  if (BTN_A) {
    BTN_A = false;
    Serial.println("BTN A");
  }
  if (BTN_B) {
    BTN_B = false;
    Serial.println("BTN B");
  }
  if (BTN_C) {
    BTN_C = false;
    Serial.println("BTN C");
  }
  if (BTN_D) {
    BTN_D = false;
    Serial.println("BTN D");
  }
}

void loop() {
  BtnQueue();
  BtnHandler();
  delay(150);  // Debounce
}
