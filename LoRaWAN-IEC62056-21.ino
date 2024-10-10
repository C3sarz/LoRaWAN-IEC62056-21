#include <Arduino.h>
#include "Peripherals.h"
#include "MeterInterface.h"
#include "LoRaWAN_Interface.h"

// Globals
extern const uint16_t fw_version;

// Uplink Parameters
extern char deviceAddress[];
extern unsigned long uplinkPeriod;
unsigned long lastRequest = millis();
unsigned long periodResult;

// Watchdog and reset variables
volatile bool setReboot = false;

// Link check variables
volatile uint linkCheckCount = 30;


void printFWInfo(){
  Serial.printf("Firmware v%.2f\r\n",((float)fw_version)/100);
  Serial.print("DevUID: [ ");
  for(int i = 0; i< sizeof(nodeDeviceEUI); i++){
    Serial.printf("0x%02hhX ", nodeDeviceEUI[i]);
  }
  Serial.println("]");
}


void setup() {
  // delay(100);
  setupPeripherals();
  delay(1000);
  // Initialize Serial for debug output
  time_t timeout = millis();
  Serial.begin(115200);
  while (!Serial) {
    if ((millis() - timeout) < 5000) {
      delay(100);
    } else {
      break;
    }
  }

  // Read config from flash
  if (!tryReadStoredConfig()) {
    Serial.println("Failed to read data config from flash!");
    while (1)
      ;
  }

  // Start RS485 and LoRa interfaces
  initMeterInterface();
  initLoRaWAN();

  // Print summary and init
  randomSeed(analogRead(WB_A0));  // Pseudorandom seed from VBAT;
  Serial.printf("Seed: %u\r\n", analogRead(WB_A0));
  periodResult = uplinkPeriod + random(0, RANDOM_TIME_DEVIATION_MAX);
  Serial.println("==========\r\nInit successful");
  printFWInfo();
  Serial.println("==========");
}

void loop() {
  processRS485();
  if (Serial.available() > 0) {

    // Serial console debug commands
    int rcvd = Serial.read();
    Serial.printf("Rcvd: %c\r\n", rcvd);
    switch (rcvd) {
      case 's':
        sendQuery(deviceAddress);
        break;
      case 'd':
        printSummary();
        printFWInfo();
        Serial.printf("Current DR: %u\r\n",getADRDatarate());
        break;
      case 't':
        {
          byte pkt[] = { 0x00 };
          sendUplink(pkt, sizeof(pkt), true);
          break;
        }
      case 'u':
        {
          byte pkt[] = { 98, 175, 221, 187, 147, 238, 76, 207, 29, 232, 168, 222, 140, 145, 242, 57, 62, 58, 168, 174, 143, 140, 51, 250, 16, 11, 143, 202, 22, 141, 37, 46, 242, 229, 158, 255, 251, 154, 222, 18, 24, 155, 67, 55, 249, 194, 246, 18, 25, 183, 143, 50, 39, 194, 253, 177, 216, 142, 64, 76, 185, 130, 124, 77 };
          sendUplink(pkt, sizeof(pkt), false);
          break;
        }
      case 'w':
        tryWriteStoredConfig();
        break;
      case 'b':
        getVBatInt();
        break;
      case 'l':
        loadDefaultConfig();
        break;
      case 'r':
        Serial.println("Reboot requested...");
        setReboot = true;
        break;
    }
  }

  // Periodical request
  else if ((millis() - lastRequest) > periodResult) {
    delay(50);
    // beepBuzzer();
    sendQuery(deviceAddress);

    periodResult = uplinkPeriod + random(0, RANDOM_TIME_DEVIATION_MAX);
    lastRequest = millis();
  }
}
