#ifndef _BATTERY_LEVEL_
#define _BATTERY_LEVEL_

#include <Arduino.h>

extern volatile bool setReboot;

uint16_t getVBatInt(void);
float readVBatFloat(void);
bool setupPeripherals(void);
void beepBuzzer(void);

#endif