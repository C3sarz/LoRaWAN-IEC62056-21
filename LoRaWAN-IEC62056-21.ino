#include "LoRaWAN_Handler.h"
#include "MeterInterface.h"


// Meter address for meters that require sign-on.
export char meterAddress[] = "76127652";

extern bool send_now;
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
  initMeterInterface();
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  // setupLoRaWAN();


  Serial.println("Init successful");
}

void loop() {
  // Every LORAWAN_APP_INTERVAL milliseconds send_now will be set
  // true by the application timer and collects and sends the data
  processRS485();
  if (Serial.available() > 0) {
    int rcvd = Serial.read();
    Serial.printf("Rcvd: %c\r\n",rcvd);
    if (rcvd == 's') {
      sendHandshake(meterAddress);
    } else if (rcvd == 'c') {
      sendHandshake("0");
    }
    
  }
  if (send_now) {
    // sendHandshake("0");
    send_now = false;
  }
}
