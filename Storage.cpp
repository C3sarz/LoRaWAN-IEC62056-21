#include "Storage.h"
#include "MeterInterface.h"
extern "C" {
#include <hardware/flash.h>
};

char deviceAddress[STRING_MAX_SIZE] = "";
unsigned long uplinkPeriod = MS_TO_M * INITIAL_PERIOD_MINUTES;
int baseBaudIndex = INITIAL_BAUD_INDEX;
FixedSizeString codes[CODES_LIMIT + 1];

const FixedSizeString debugCodes[CODES_LIMIT + 1] = {
  "32.7.0",
  "52.7.0",
  "72.7.0",
  "0.9.2*255",
  "15.8.0*02",
};

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
    currentAddrOffset += STRING_MAX_SIZE;
  }

  return true;
}

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
  else if (opcode == SAVE_CHANGES) {
    return writeToStorage();
  }
  return false;
}

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