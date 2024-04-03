#include <sys/_stdint.h>
#include "Peripherals.h"


const uint32_t vbat_pin = PIN_VBAT;

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