#include <cstring>
#include "StorageInterface.h"
#include "MeterInterface.h"
extern "C" {
#include <hardware/flash.h>
};

/// IEC62056-21 Meter Address, mutable
char deviceAddress[STRING_MAX_SIZE] = "";

// DEFAULT
const char defaultDeviceAddress[] = "";

/// Device UPLINK period in milliseconds
unsigned long uplinkPeriod = MINUTE_IN_MS * INITIAL_PERIOD_MINUTES;

/// Base baud rate index
int currentBaudIndex = DEFAULT_BAUD_INDEX;

/// Loaded OBIS codes
CodeString loadedCodes[CODES_LIMIT + 1];

/*
  Load default device parameters (saved as constants in code).
*/
void loadDefaultConfig() {
  uplinkPeriod = MINUTE_IN_MS * INITIAL_PERIOD_MINUTES;
  currentBaudIndex = DEFAULT_BAUD_INDEX;
  strcpy(deviceAddress, defaultDeviceAddress);
  for (int i = 0; i < CODES_LIMIT; i++) {
    memset(loadedCodes[i], 0, STRING_MAX_SIZE);
    strcpy(loadedCodes[i], defaultCodes[i]);
  }
  Serial.println("Default config loaded...");
}

/* 
  Write device parameters into nonvolatile storage. 
  WARNING: Will wear down flash if not used properly and sparingly.
*/
bool tryWriteStoredConfig(void) {
  StoredConfig config;

  // Pack data to object
  byte uplinkMinutes = static_cast<byte>(uplinkPeriod / MINUTE_IN_MS);
  config.uplinkMinutes = uplinkMinutes;
  config.baudIndex = currentBaudIndex;
  for (unsigned int i = 0; i < CODES_LIMIT; i++) {
    memcpy(config.codes[i], loadedCodes[i], STRING_MAX_SIZE);
  }
  return (IWriteStoredValues(config));
}

// Read saved settings from nonvolatile storage
bool tryReadStoredConfig(void) {
  StoredConfig config;
  if (IReadStoredValues(&config)) {
    byte uplinkMinutes = config.uplinkMinutes;
    uplinkPeriod = uplinkMinutes * MINUTE_IN_MS;
    currentBaudIndex = config.baudIndex;
    for (int i = 0; i < CODES_LIMIT; i++) {
      memcpy(loadedCodes[i], config.codes[i], STRING_MAX_SIZE);
      loadedCodes[i][15] = '\0';
    }
    return true;
  }
  return false;
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
    uplinkPeriod = MINUTE_IN_MS * parameter;
    return true;
  }

  // Change RS485 baud speed (index class C)
  else if (opcode == CHANGE_SERIAL_BAUD && parameter < 7) {
    if (changeBaudRS485(parameter)) {
      Serial.printf("Baud changed to %u", parameter);
      currentBaudIndex = parameter;
    }
    return true;
  }

  // Update individual OBIS code
  else if (opcode == UPDATE_CODE && parameter < CODES_LIMIT) {
    if (dataLen > 0
        && dataLen < (STRING_MAX_SIZE)
        && (bufLen >= 2 + dataLen)) {
      memcpy(loadedCodes[parameter], buffer + 2, dataLen);
      loadedCodes[parameter][dataLen] = '\0';
      return true;
    }
  }

  // Change devices address
  else if (opcode == CHANGE_DEVICE_ADDR) {
    if (dataLen > 0
        && dataLen < (STRING_MAX_SIZE)
        && (bufLen >= 2 + dataLen)) {
      memcpy(deviceAddress, buffer + 2, dataLen);
      deviceAddress[dataLen] = '\0';
      return true;
    }
  }

  // Save device properties to nonvolatile storage
  else if (opcode == SAVE_CHANGES) {
    return tryWriteStoredConfig();
  }

  // Reboot command
  else if (opcode == REBOOT) {
    if (parameter == 0xFF) {
      setReboot = true;
      Serial.println("Reboot requested...");
      return true;
    }
  }
  return false;
}

// Print summary of device parameters on serial console
void printSummary() {
  Serial.println("=========\r\nCODES:");
  for (unsigned int i = 0; i < CODES_LIMIT; i++) {
    Serial.printf("Code #%d: \"%s\"\r\n", i, loadedCodes[i]);
  }
  Serial.printf("Device Address: %s\r\n", deviceAddress);
  Serial.printf("Baud speed index: %u\r\n", currentBaudIndex);
  Serial.printf("Device will report every %lu minutes.\r\n", uplinkPeriod / MINUTE_IN_MS);
  Serial.println("=========");
}