#include "Peripherals.h"
#include "config.h"
extern "C" {
#include "hardware/watchdog.h"
}
#include "mbed.h"
#include "rtos.h"

#define VBATFILTERSIZE 50
#define PIN_VBAT WB_A0
#define VBAT_MV_PER_LSB (0.806F)    // 3.0V ADC range and 12 - bit ADC resolution = 3300mV / 4096
#define VBAT_DIVIDER (0.6F)         // 1.5M + 1M voltage divider on VBAT = (1.5M / (1M + 1.5M))
#define VBAT_DIVIDER_COMP (1.846F)  //  // Compensation factor for the VBAT divider
#define REAL_VBAT_MV_PER_LSB (VBAT_DIVIDER_COMP * VBAT_MV_PER_LSB)
#define BUZZER_CONTROL WB_IO2
const uint32_t vbat_pin = PIN_VBAT;
const unsigned long TIMER_ISR_PERIOD_MS = 1000;


//==================================================================
//==================================================================
//==================================================================
//==================================================================

mbed::Ticker watchdogTimer;
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


bool setupPeripherals(void) {
  analogReadResolution(12);

  // LED Setup
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);
  pinMode(BUZZER_CONTROL, OUTPUT);
  digitalWrite(PIN_LED1, 1);
  digitalWrite(PIN_LED2, 1);
  tone(BUZZER_CONTROL, 500);
  delay(100);
  noTone(BUZZER_CONTROL);


  // Watchdog Init
  watchdog_enable(5000, false);
  watchdogTimer.attach(ISR_WatchdogRefresh, (std::chrono::microseconds)(TIMER_ISR_PERIOD_MS * 1000));

  return true;
}

void beepBuzzer(void) {
  tone(BUZZER_CONTROL, 500);
  delay(300);
  noTone(BUZZER_CONTROL);
}

/**
 * @brief Get RAW Battery Voltage, from RAK WIRELESS
 */
float readVBatFloat(void) {
  unsigned int sum = 0, average_value = 0;
  unsigned int read_temp[10] = { 0 };
  unsigned char i = 0;
  unsigned int adc_max = 0;
  unsigned int adc_min = 4095;
  average_value = analogRead(vbat_pin);
  for (i = 0; i < 10; i++) {
    read_temp[i] = analogRead(vbat_pin);
    if (read_temp[i] < adc_min) {
      adc_min = read_temp[i];
    }
    if (read_temp[i] > adc_max) {
      adc_max = read_temp[i];
    }
    sum = sum + read_temp[i];
  }
  average_value = (sum - adc_max - adc_min) >> 3;
  // Serial.printf("The ADC value is:%d\r\n",average_value);

  // Convert the raw value to compensated mv, taking the resistor-
  // divider into account (providing the actual LIPO voltage)
  // ADC range is 0..3300mV and resolution is 12-bit (0..4095)
  float volt = average_value * REAL_VBAT_MV_PER_LSB;

  // Serial.printf("The battery voltage is: %3.2f V\r\n",volt);
  return volt;
}

uint16_t getVBatInt(void) {
  if (!BATTERY_INSTALLED) {
    return 0;
  }
  float voltage = readVBatFloat();
  uint16_t voltageInt = static_cast<uint16_t>(voltage);
  voltageInt -= 100;  //offset
  Serial.printf("VBAT: %u\r\n", voltageInt);
  return voltageInt;
}

// byte getVBatPercent(void){
//   uint16_t vbat = getVBatInt();
//   byte value = 0;
//   if(vbat >= 3800)
//     value = 90;

//   else if(vbat >= 3750)
//   value = 80;

// }