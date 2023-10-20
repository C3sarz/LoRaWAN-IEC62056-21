#include <cstring>
#include "Storage.h"
#include "MeterInterface.h"
extern "C" {
#include <hardware/flash.h>
};

/// IEC62056-21 Meter Address, mutable
char deviceAddress[STRING_MAX_SIZE] = "";

// DEFAULT
const char defaultDeviceAddress[] = "";

/// Device UPLINK period in milliseconds
unsigned long uplinkPeriod = MS_TO_M * INITIAL_PERIOD_MINUTES;

/// Base baud rate index
int baseBaudIndex = INITIAL_BAUD_INDEX;

/// Loaded OBIS codes
FixedSizeString codes[CODES_LIMIT + 1];

const char* defaultCodes[] = {
  "128.8.10",
  "128.8.20",
  "128.8.30",
  "128.8.12",
  "128.8.22",
  "128.8.32",
  "32.7.0",
  "52.7.0",
  "72.7.0",
  "15.7.0",
  "15.5.0",
  "15.8.0",
  "15.6.0",
  "13.5.0",
  "",
  ""
};

/*
  Load default device parameters (saved as constants in code).
*/
void loadDefaultValues() {
  uplinkPeriod = MS_TO_M * INITIAL_PERIOD_MINUTES;
  baseBaudIndex = INITIAL_BAUD_INDEX;
  strcpy(deviceAddress, defaultDeviceAddress);
  for (int i = 0; i < CODES_LIMIT; i++) {
    memset(codes[i], 0, STRING_MAX_SIZE);
    strcpy(codes[i], defaultCodes[i]);
  }
  Serial.println("Default values loaded...");
}

/*
 Check if loaded device parameters are different
  to the stored parameters in flash
 */
bool dataHasChanged() {
  const byte* DATA_BASE = (const byte*)XIP_BASE + STORAGE_FLASH_OFFSET;
  char currentStr[STRING_MAX_SIZE];
  unsigned int currentAddrOffset = 0;

  // Compare uplink period
  if (*DATA_BASE != static_cast<byte>(uplinkPeriod / MS_TO_M)) {
    Serial.printf("O: %u, N: %u\r\n", *DATA_BASE, static_cast<byte>(uplinkPeriod / MS_TO_M));
    return true;
  }
  currentAddrOffset++;

  // Compare baud index
  if (*(DATA_BASE + currentAddrOffset) != baseBaudIndex) {
    Serial.printf("O: %u, N: %u\r\n", *(DATA_BASE + currentAddrOffset), baseBaudIndex);
    return true;
  }
  currentAddrOffset++;

  // Compare device address
  memcpy(currentStr, DATA_BASE + currentAddrOffset, STRING_MAX_SIZE);
  if (strcmp(currentStr, deviceAddress) != 0) {
    Serial.printf("O: %s, N: %s\r\n", currentStr, deviceAddress);
    return true;
  }
  currentAddrOffset += STRING_MAX_SIZE;

  // Compare codes
  for (int i = 0; i < CODES_LIMIT; i++) {
    memcpy(currentStr, DATA_BASE + currentAddrOffset, STRING_MAX_SIZE);
    if (strcmp(currentStr, codes[i]) != 0) {
      Serial.printf("O: %s, N: %s\r\n", currentStr, codes[i]);
      return true;
    }
    currentAddrOffset += STRING_MAX_SIZE;
  }
  return false;
}

/* 
  Write device parameters into nonvolatile storage. 
  WARNING: Will wear down flash if not used properly and sparingly.
*/
bool writeToStorage() {

  // Check if there are any changes
  if (!dataHasChanged()) {
    Serial.println("No change in data detected");
    return false;
  }
  Serial.println("Data change detected");

  // Serialize data
  byte storageBuffer[STORAGE_SIZE];
  byte uplinkMinutes = static_cast<byte>(uplinkPeriod / MS_TO_M);
  unsigned int currentAddrOffset = 2;
  memcpy(storageBuffer, &uplinkMinutes, 1);
  memcpy(storageBuffer + 1, &baseBaudIndex, 1);
  memcpy(storageBuffer + currentAddrOffset, deviceAddress, STRING_MAX_SIZE);
  currentAddrOffset += STRING_MAX_SIZE;
  for (int i = 0; i < CODES_LIMIT; i++) {
    memcpy(storageBuffer + currentAddrOffset, codes[i], STRING_MAX_SIZE);
    currentAddrOffset += STRING_MAX_SIZE;
  }

  // Erase flash sector and write
  noInterrupts();
  flash_range_erase(STORAGE_FLASH_OFFSET, FLASH_SECTOR_SIZE);
  flash_range_program(STORAGE_FLASH_OFFSET, storageBuffer, FLASH_SECTOR_SIZE);
  interrupts();
  return true;
}

// Read saved settings from nonvolatile storage
bool readFromStorage() {
  codes[CODES_LIMIT][0] = 0;
  const byte* DATA_BASE = (const byte*)XIP_BASE + STORAGE_FLASH_OFFSET;
  unsigned int currentAddrOffset = 0;

  byte uplinkMinutes = INITIAL_PERIOD_MINUTES;
  memcpy(&uplinkMinutes, DATA_BASE, 1);
  uplinkPeriod = uplinkMinutes * MS_TO_M;
  currentAddrOffset++;

  memcpy(&baseBaudIndex, DATA_BASE + currentAddrOffset, 1);
  currentAddrOffset++;

  memcpy(deviceAddress, DATA_BASE + currentAddrOffset, STRING_MAX_SIZE);
  currentAddrOffset += STRING_MAX_SIZE;

  for (int i = 0; i < CODES_LIMIT; i++) {
    memcpy(codes[i], DATA_BASE + currentAddrOffset, STRING_MAX_SIZE);
    codes[i][15] = '\0';
    currentAddrOffset += STRING_MAX_SIZE;
  }
  return true;
}

// Process downlink commands
bool processDownlinkPacket(byte* buffer, byte bufLen) {

  for (int i = 0; i < bufLen; i++) {
    Serial.printf("0x%x ", buffer[i]);
  }
  Serial.println("");

  if (bufLen < 3) {
    return false;
  }
  byte opcode = buffer[0];
  byte parameter = buffer[1];
  byte dataLen = buffer[2];
  Serial.printf("OP:%u, PRM:%u, LEN:%u\r\n", opcode, parameter, dataLen);

  // Update Tx Period in minutes
  if (opcode == UPDATE_PERIOD && parameter > 0) {
    uplinkPeriod = MS_TO_M * parameter;
    return true;
  }

  // Change RS485 baud speed (index class C)
  else if (opcode == CHANGE_SERIAL_BAUD && parameter < 7) {
    if (changeBaud(parameter)) {
      Serial.printf("Baud changed to %u", parameter);
      baseBaudIndex = parameter;
    }
    return true;
  }

  // Update individual OBIS code
  else if (opcode == UPDATE_CODE && parameter < CODES_LIMIT) {
    if (dataLen > 0
        && dataLen < (STRING_MAX_SIZE)
        && (bufLen >= HEADER_LENGTH + dataLen)) {
      memcpy(codes[parameter], buffer + HEADER_LENGTH, dataLen);
      codes[parameter][dataLen] = '\0';
      return true;
    }
  }

  // Change devices address
  else if (opcode == CHANGE_DEVICE_ADDR) {

    if (dataLen > 0
        && dataLen < (STRING_MAX_SIZE)
        && (bufLen >= HEADER_LENGTH + dataLen)) {
      memcpy(deviceAddress, buffer + HEADER_LENGTH, dataLen);
      deviceAddress[dataLen] = '\0';
      return true;
    }
  }

  // Save device properties to nonvolatile storage
  else if (opcode == SAVE_CHANGES) {
    return writeToStorage();
  }
  return false;
}

// Print summary of device parameters on serial console
void printSummary() {
  Serial.println("=========\r\nCODES:");
  for (unsigned int i = 0; i < CODES_LIMIT; i++) {
    Serial.printf("Code #%d: \"%s\"\r\n", i, codes[i]);
  }
  Serial.printf("Device Address: %s\r\n", deviceAddress);
  Serial.printf("Baud speed index: %u\r\n", baseBaudIndex);
  Serial.printf("Device will report every %lu minutes.\r\n", uplinkPeriod / MS_TO_M);
  Serial.println("=========");
}