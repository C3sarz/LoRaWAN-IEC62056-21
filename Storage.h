/* 
  EEPROM layout, starting at START_ADDRESS.
  ========================================================================
  | periodM (1B) | baudIndx (1B) | address (16B)  | code n... |
  ========================================================================
*/

#ifndef _STORAGE_HANDLER_
#define _STORAGE_HANDLER_

#include <Arduino.h>
extern "C" {
#include <hardware/flash.h>
};

#define STRING_MAX_SIZE 16
#define INITIAL_BAUD_INDEX 0
#define INITIAL_PERIOD_MINUTES 1
#define CODES_LIMIT 16
#define HEADER_LENGTH 3
#define STORAGE_SIZE 2 + STRING_MAX_SIZE + (STRING_MAX_SIZE * CODES_LIMIT)
#define STORAGE_FLASH_OFFSET PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE  // last sector in flash

const unsigned long MS_TO_M = 1000 * 60;
typedef char FixedSizeString[STRING_MAX_SIZE];

enum Downlink_Operation {
  UPDATE_PERIOD = 0x01,
  UPDATE_CODE,
  CHANGE_DEVICE_ADDR,
  CHANGE_SERIAL_BAUD,
  SAVE_CHANGES
};

bool readFromStorage();
bool writeToStorage();
bool processDownlinkPacket(byte* buffer, byte bufLen);
void printSummary();
bool dataHasChanged();
void loadDefaultValues();

#endif