extern "C" {
#include "hardware/watchdog.h"
}
#include <Arduino.h>
#include "Storage.h"
#include "LoRaWAN_Handler.h"
#include "MeterInterface.h"
#include "mbed.h"
#include "rtos.h"

// Uplink Parameters
extern char deviceAddress[];
extern unsigned long uplinkPeriod;
unsigned long lastRequest = millis();
unsigned long periodResult;

// Watchdog and reset variables
volatile bool setReboot = false;
mbed::Ticker watchdogTimer;
const unsigned long TIMER_ISR_PERIOD_MS = 1000;

// Link check variables
volatile uint linkCheckCount = 30;

//==================================================================
//==================================================================
//==================================================================
//==================================================================

void ISR_WatchdogRefresh(void) {
  static bool toggle = false;

  ///////////////////////////////////////////////////////////

  if (!setReboot) {
    watchdog_update();
    digitalWrite(LED_BUILTIN, toggle);
  }
  toggle = !toggle;
  watchdogTimer.attach(ISR_WatchdogRefresh, (std::chrono::microseconds)(TIMER_ISR_PERIOD_MS * 1000));

  ////////////////////////////////////////////////////////////
}

//==================================================================
//==================================================================
//==================================================================
//==================================================================

void setup() {

  // LED Setup
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

  // Watchdog Init
  watchdog_enable(5000, false);
  watchdogTimer.attach(ISR_WatchdogRefresh, (std::chrono::microseconds)(TIMER_ISR_PERIOD_MS * 1000));

  // Read config from flash
  if (!readFromStorage()) {
    Serial.println("Failed to read data config from flash!");
    while (1)
      ;
  }

  // Start RS485 and LoRa interfaces
  initMeterInterface();
  setupLoRaWAN();

  // Set random seed from analog voltage in A0.
  analogReadResolution(12);
  randomSeed(analogRead(WB_A0));  // Pseudorandom seed from VBAT;

  // Print summary and init
  Serial.println("\r\n==========================\r\nInit successful");
  Serial.printf("Seed: %u\r\n", analogRead(WB_A0));
  periodResult = uplinkPeriod + random(0, RANDOM_TIME_MAX);
  // printSummary();
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
        break;
      case 't':
        {
          byte pkt[] = { 0x00 };
          send_lora_frame(pkt, sizeof(pkt), true);
          break;
        }
      case 'u':
        {
          byte pkt[] = { 98, 175, 221, 187, 147, 238, 76, 207, 29, 232, 168, 222, 140, 145, 242, 57, 62, 58, 168, 174, 143, 140, 51, 250, 16, 11, 143, 202, 22, 141, 37, 46, 242, 229, 158, 255, 251, 154, 222, 18, 24, 155, 67, 55, 249, 194, 246, 18, 25, 183, 143, 50, 39, 194, 253, 177, 216, 142, 64, 76, 185, 130, 124, 77 };
          send_lora_frame(pkt, sizeof(pkt), false);
          break;
        }
      case 'w':
        writeToStorage();
        break;
      case 'q':
        Serial.printf("Result: %d\r\n", dataHasChanged());
        break;
      case 'b':
        getVBatInt();
        break;
      case 'l':
        loadDefaultValues();
        break;
      case 'r':
        Serial.println("Reboot requested...");
        setReboot = true;
        break;
    }
  }

  // Periodical request
  else if ((millis() - lastRequest) > periodResult) {
    if (lmh_join_status_get() == LMH_SET) {

      changeBaud(DEFAULT_BAUD_INDEX);
      delay(50);
      sendQuery(deviceAddress);
    } else {
      Serial.println("Device has not joined network yet, cancel request...");
    }
    periodResult = uplinkPeriod + random(0, RANDOM_TIME_MAX);
    lastRequest = millis();
  }
}
