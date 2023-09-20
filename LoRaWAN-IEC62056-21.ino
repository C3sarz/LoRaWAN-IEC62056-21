#include "LoRaWAN_Handler.h"
#include "MeterInterface.h"

#define REQUEST_PERIOD (1000 * 60) * 1

// Parameters
const char meterAddress[] = "86903638";
unsigned long lastRequest = millis();
unsigned long requestPeriod = REQUEST_PERIOD;

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

  // Start RS485 and LoRa interfaces
  initMeterInterface();
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  setupLoRaWAN();
  Serial.println("Init successful");
}

void loop() {
  processRS485();
  if (Serial.available() > 0) {
    int rcvd = Serial.read();
    Serial.printf("Rcvd: %c\r\n", rcvd);
    if (rcvd == 's') {
      sendHandshake(meterAddress);
    } else if (rcvd == 'c') {
      sendHandshake(NOADDRESS);
    } else if (rcvd == 't') {
      byte pkt[] = { 0x1f, 0xff, 0x03, 0x3d, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0xfb, 0x00, 0x00, 0x09, 0x0c, 0x00, 0x00, 0x09, 0x05, 0x00, 0x0e, 0x06, 0x75, 0x00, 0x03, 0x58, 0x4a, 0x00, 0x07, 0xc5, 0xd2, 0x00, 0x0e, 0xab, 0x05 };
      send_lora_frame(pkt, sizeof(pkt));
    }
  }
  // Status request
  else if ((millis() - lastRequest) > requestPeriod) {
    if (lmh_join_status_get() == LMH_SET) {
      sendHandshake(meterAddress);
    } else {
      lmh_join();
    }
    lastRequest = millis();
  }
}
