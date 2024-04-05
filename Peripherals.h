#ifndef _BATTERY_LEVEL_
#define _BATTERY_LEVEL_

#include <Arduino.h>

#define VBATFILTERSIZE 50
#define PIN_VBAT WB_A0
#define VBAT_MV_PER_LSB (0.806F)    // 3.0V ADC range and 12 - bit ADC resolution = 3300mV / 4096
#define VBAT_DIVIDER (0.6F)         // 1.5M + 1M voltage divider on VBAT = (1.5M / (1M + 1.5M))
#define VBAT_DIVIDER_COMP (1.846F)  //  // Compensation factor for the VBAT divider
#define REAL_VBAT_MV_PER_LSB (VBAT_DIVIDER_COMP * VBAT_MV_PER_LSB)

uint16_t getVBatInt(void);
float readVBatFloat(void);

#endif