/* 
  EEPROM layout, starting at STORAGE_FLASH_OFFSET.
  ========================================================================
  | periodM (1B) | baudIndx (1B) | address (16B)  | code n... |
  ========================================================================
*/

#ifndef _STORAGE_HANDLER_
#define _STORAGE_HANDLER_

#include "config.h"
#include "StorageImplementation.h"

const unsigned long MINUTE_IN_MS = 1000 * 60;
typedef char CodeString[STRING_MAX_SIZE];
extern const char* defaultCodes[];

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

bool tryReadStoredConfig(void);
bool tryWriteStoredConfig(void);
bool processDownlinkPacket(byte* buffer, byte bufLen);
void printSummary(void);
void loadDefaultConfig(void);

#endif