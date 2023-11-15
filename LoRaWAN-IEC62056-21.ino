#include <Arduino.h>
#include "Storage.h"
#include "LoRaWAN_Handler.h"
#include "MeterInterface.h"
extern "C" {
#include "hardware/watchdog.h"
  // #include <Radio.h>
}

// Parameters
extern char deviceAddress[];
extern unsigned long uplinkPeriod;
unsigned long lastRequest = millis();
unsigned long periodResult;

void setup() {
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);
  digitalWrite(PIN_LED1, 1);
  digitalWrite(PIN_LED2, 1);
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

  // Read config and codes
  if (!readFromStorage()) {
    Serial.println("Failed to read data config from flash!");
    while (1)
      ;
  }

  // Start RS485 and LoRa interfaces
  initMeterInterface();
  setupLoRaWAN();
  Serial.println("\r\n==========================\r\nInit successful");
  periodResult = uplinkPeriod + random(0, ALOHA_RANDOM_TIME_MAX);
  printSummary();
}

void loop() {
  processRS485();
  if (Serial.available() > 0) {
    int rcvd = Serial.read();
    Serial.printf("Rcvd: %c\r\n", rcvd);
    switch (rcvd) {
      case 's':
        sendHandshake(deviceAddress);
        break;
      case 'c':
        sendHandshake(NOADDRESS);
        break;
      case 'd':
        printSummary();
        break;
      case 't':
        {
          byte pkt[] = { 0x00 };
          send_lora_frame(pkt, sizeof(pkt), true);
          break;
        }
      case 'u':
        {
          byte pkt[] = {98, 175, 221, 187, 147, 238, 76, 207, 29, 232, 168, 222, 140, 145, 242, 57, 62, 58, 168, 174, 143, 140, 51, 250, 16, 11, 143, 202, 22, 141, 37, 46, 242, 229, 158, 255, 251, 154, 222, 18, 24, 155, 67, 55, 249, 194, 246, 18, 25, 183, 143, 50, 39, 194, 253, 177, 216, 142, 64, 76, 185, 130, 124, 77};
          send_lora_frame(pkt, sizeof(pkt), false);
          break;
        }
      case 'w':
        writeToStorage();
        break;
      case 'q':
        Serial.printf("Result: %d\r\n", dataHasChanged());
        break;
      case 'l':
        loadDefaultValues();
        break;
    }
  }

  // Periodical request
  else if ((millis() - lastRequest) > periodResult) {
    if (lmh_join_status_get() == LMH_SET) {
      sendHandshake(deviceAddress);
    } else {
      Serial.println("Device has not joined network yet, cancel request...");
    }
    periodResult = uplinkPeriod + random(0, ALOHA_RANDOM_TIME_MAX);
    lastRequest = millis();
  }
}
