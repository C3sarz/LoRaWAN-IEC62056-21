#include "Storage.h"
#include "MeterInterface.h"

char deviceAddress[STRING_MAX_SIZE] = "";
unsigned long requestPeriod = MS_TO_M * INITIAL_PERIOD_MINUTES;
char baseBaudIndex = INITIAL_BAUD_INDEX;
FixedSizeString codes[CODES_LIMIT + 1];

const FixedSizeString debugCodes[CODES_LIMIT + 1] = {
  "32.7.0",
  "52.7.0",
  "72.7.0",
  "0.9.2*255",
  "15.8.0*02",
};

bool readFromStorage() {

  codes[CODES_LIMIT][0] = 0;
  for (int i = 0; i < CODES_LIMIT && debugCodes[i][0] != '\0'; i++) {
    strcpy(codes[i], debugCodes[i]);
  }
  memcpy(deviceAddress, "0", 2);



  //   EEPROM.begin(3 + 16 * STRING_MAX_SIZE);
  //   // Reporting period
  //   periodMinutes = EEPROM.read(START_ADDRESS) * MS_TO_M;

  //   // RS485 Baud index
  //   currentBaudIndex = EEPROM.read(START_ADDRESS + 1);

  //   // Device address
  //   for (int i = 0; i < STRING_MAX_SIZE; i++) {
  //     deviceAddress[i] = static.cast<char>(EEPROM.read((START_ADDRESS + 2) + i));
  //   }

  //   // OBIS Codes
  //   for (int codeIndex = 0; codeIndex < CODES_LIMIT; codeIndex++) {
  //     unsigned int currentOffset = CODES_START_ADDR + (codeIndex * STRING_MAX_SIZE);
  //     for (int i = 0; i < STRING_MAX_SIZE; i++) {
  //       codes[codeIndex][i] = static.cast<char>(EEPROM.read(currentOffset + i));
  //     }
  //     codes[codeIndex][STRING_MAX_SIZE - 1] = '\0';
  //   }

  //   codes[16] = NULL;
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
    requestPeriod = MS_TO_M * parameter;
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
  return false;
}

void printSummary() {
  Serial.println("=========\r\nCODES:");
  for (unsigned int i = 0; i < CODES_LIMIT; i++) {
    Serial.printf("Code #%d: %s\r\n", i, codes[i]);
  }
  Serial.printf("Device Address: %s\r\n", deviceAddress);
  Serial.printf("Baud speed index: %u\r\n", baseBaudIndex);
  Serial.printf("Device will report every %lu minutes.\r\n", requestPeriod / MS_TO_M);
  Serial.println("=========");
}