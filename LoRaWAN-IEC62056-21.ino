#include <Arduino.h>
#include "Storage.h"
#include "LoRaWAN_Handler.h"
#include "MeterInterface.h"

// Parameters
extern char deviceAddress[];
extern unsigned long uplinkPeriod;
unsigned long lastRequest = millis();

void setup() {
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
    Serial.println("Failed to read data config from EEPROM!");
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
}

void loop() {
  processRS485();
  if (Serial.available() > 0) {
    int rcvd = Serial.read();
    Serial.printf("Rcvd: %c\r\n", rcvd);
    if (rcvd == 's') {
      sendHandshake(deviceAddress);
    } else if (rcvd == 'c') {
      sendHandshake(NOADDRESS);
    } else if (rcvd == 'd') {
      printSummary();
    } else if (rcvd == 't') {
      byte pkt[] = { 0x1f, 0xff, 0x03, 0x3d, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0xfb, 0x00, 0x00, 0x09, 0x0c, 0x00, 0x00, 0x09, 0x05, 0x00, 0x0e, 0x06, 0x75, 0x00, 0x03, 0x58, 0x4a, 0x00, 0x07, 0xc5, 0xd2, 0x00, 0x0e, 0xab, 0x05 };
      send_lora_frame(pkt, sizeof(pkt));
    } else if (rcvd == 'w') {
      writeToStorage();
    } else if (rcvd == 'q') {
      Serial.printf("Result: %d\r\n",dataHasChanged());
    } else if (rcvd == 'l') {
      loadDefaultValues();
    } 
  }
  // Status request
  else if ((millis() - lastRequest) > uplinkPeriod) {
    if (lmh_join_status_get() == LMH_SET) {
      sendHandshake(deviceAddress);
    } else {
      lmh_join();
    }
    lastRequest = millis();
  }
}
