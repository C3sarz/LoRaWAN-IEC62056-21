#include "StorageImplementation.h"
// #define STORAGE_SIZE 2 + STRING_MAX_SIZE + (STRING_MAX_SIZE * CODES_LIMIT)
#define STORAGE_FLASH_OFFSET PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE  // last sector in flash
extern "C" {
#include <hardware/flash.h>
};

const unsigned int MAX_STORAGE_SIZE = 500;

/*
 Check if loaded device parameters are different
  to the stored parameters in flash
 */
bool checkIfDataChanged(StoredConfig config) {
  const byte* DATA_BASE = (const byte*)XIP_BASE + STORAGE_FLASH_OFFSET;

  unsigned int size = sizeof(StoredConfig);
  if (size > MAX_STORAGE_SIZE) {
    return false;
  }

  byte* incomingConfigPtr = (byte*)&config;

  for (unsigned int i = 0; i < size; i++) {
    if (incomingConfigPtr[i] != DATA_BASE[i]) {
      return true;
    }
  }
  return false;
}

/* 
  Write device parameters into nonvolatile storage. 
  WARNING: Will wear down flash if not used properly and sparingly.
*/
bool IWriteStoredValues(StoredConfig config) {

  // Check if there are any changes
  if (!checkIfDataChanged(config)) {
    Serial.println("No change in data detected");
    return false;
  }
  Serial.println("Data change detected");

  unsigned int size = sizeof(StoredConfig);
  byte* configPtr = (byte*)&config;
  if (size > MAX_STORAGE_SIZE) {
    return false;
  }

  // Erase flash sector and write
  noInterrupts();
  flash_range_erase(STORAGE_FLASH_OFFSET, FLASH_SECTOR_SIZE);
  flash_range_program(STORAGE_FLASH_OFFSET, configPtr, FLASH_SECTOR_SIZE);
  interrupts();
  return true;
}

// Read saved settings from nonvolatile storage
bool IReadStoredValues(StoredConfig* config) {
  const byte* DATA_BASE = (const byte*)XIP_BASE + STORAGE_FLASH_OFFSET;
  unsigned int size = sizeof(StoredConfig);
  if (size > MAX_STORAGE_SIZE) {
    return false;
  }
  memcpy(config, DATA_BASE, size);
  return true;
}
