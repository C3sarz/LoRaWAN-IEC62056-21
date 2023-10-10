#include <Arduino.h>
#include "Storage.h"
#include "LoRaWAN_Handler.h"
#include "MeterInterface.h"

// Parameters
extern char deviceAddress[];
extern unsigned long uplinkPeriod;
unsigned long lastRequest = millis();

void setup() {
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);
  digitalWrite(PIN_LED1,1);
  digitalWrite(PIN_LED2,1);
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
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  setupLoRaWAN();
  Serial.println("\r\n==========================\r\nInit successful");
  printSummary();
  digitalWrite(PIN_LED1,0);
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
          send_lora_frame(pkt, sizeof(pkt));
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
  else if ((millis() - lastRequest) > uplinkPeriod) {
    if (lmh_join_status_get() == LMH_SET) {
      sendHandshake(deviceAddress);
    } else {
      Serial.println("Device has not joined network yet, cancel request...");
    }
    lastRequest = millis();
  }
}
