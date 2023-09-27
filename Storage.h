#ifndef _STORAGE_HANDLER_
#define _STORAGE_HANDLER_

// #include <Arduino.h>
#include <string.h>

#define START_ADDRESS 0
#define CODES_START_ADDR START_ADDRESS + 2 + STRING_MAX_SIZE
#define STRING_MAX_SIZE 16
#define INITIAL_BAUD_INDEX 0
#define INITIAL_PERIOD_MINUTES 1
#define CODES_LIMIT 16
#define HEADER_LENGTH 3

typedef unsigned char byte;

typedef char FixedSizeString[STRING_MAX_SIZE];

enum Downlink_Operation {
  UPDATE_PERIOD,
  UPDATE_CODE,
  CHANGE_DEVICE_ADDR,
  CHANGE_SERIAL_BAUD
};
/* 
  EEPROM layout, starting at START_ADDRESS.
  ========================================================================
  | periodM (1B) | baudIndx (1B) | address (16B)  | code n... |
  ========================================================================
*/


void readFromStorage();
void processDownlinkPacket(byte* buffer, byte bufLen);

#endif