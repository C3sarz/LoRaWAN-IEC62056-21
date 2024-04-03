/* 
  EEPROM layout, starting at STORAGE_FLASH_OFFSET.
  ========================================================================
  | periodM (1B) | baudIndx (1B) | address (16B)  | code n... |
  ========================================================================
*/

#ifndef _STORAGE_HANDLER_
#define _STORAGE_HANDLER_

#include <Arduino.h>
#include "config.h"
extern "C" {
#include <hardware/flash.h>
};

#define STRING_MAX_SIZE 16
#define HEADER_LENGTH 3
#define STORAGE_SIZE 2 + STRING_MAX_SIZE + (STRING_MAX_SIZE * CODES_LIMIT)
#define STORAGE_FLASH_OFFSET PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE  // last sector in flash

const unsigned long MINUTE_IN_MS = 1000 * 60;
typedef char CodeString[STRING_MAX_SIZE];

// Globals
extern volatile uint linkCheckCount;
extern volatile bool setReboot;

enum Downlink_Operation {
  UPDATE_PERIOD = 0x01,
  UPDATE_CODE,
  CHANGE_DEVICE_ADDR,
  CHANGE_SERIAL_BAUD,
  SAVE_CHANGES,
  REBOOT,
};

bool readFromStorage();
bool writeToStorage();
bool processDownlinkPacket(byte* buffer, byte bufLen);
void printSummary();
bool dataHasChanged();
void loadDefaultValues();

#endif