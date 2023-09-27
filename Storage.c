#include "Storage.h"

char deviceAddress[STRING_MAX_SIZE] = "";
byte periodMinutes = INITIAL_PERIOD_MINUTES;
char currentBaudIndex = INITIAL_BAUD_INDEX;
FixedSizeString codes[CODES_LIMIT + 1];

const FixedSizeString debugCodes[CODES_LIMIT + 1] = {
  "32.7.0",
  "52.7.0",
  "72.7.0",
  "0.9.2*255",
  "15.8.0*02"
};

void readFromStorage() {

  for (int i = 0; i < CODES_LIMIT + 1 && debugCodes[i] != NULL; i++) {
    strcpy(codes[i], debugCodes[i]);
  }



  //   EEPROM.begin(3 + 16 * STRING_MAX_SIZE);
  //   // Reporting period
  //   periodMinutes = EEPROM.read(START_ADDRESS);

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
  // return true;
}

void processDownlinkPacket(byte* buffer, byte bufLen) {
  if (bufLen < 3) {
    // return false;
  }

  byte opcode = buffer[0];
  byte parameter = buffer[1];
  byte dataLen = buffer[2];

  // Update Tx Period in minutes
  if (opcode == UPDATE_PERIOD && parameter > 0) {
    periodMinutes = parameter;
    // return true;
  }

  // Change RS485 baud speed (index class C)
  else if (opcode == UPDATE_PERIOD && parameter < 7) {
    currentBaudIndex = parameter;
    // return true;
  }

  // Update individual OBIS code
  else if (opcode == UPDATE_CODE && parameter < CODES_LIMIT - 1) {
    if (dataLen > 0
        && dataLen < STRING_MAX_SIZE
        && (bufLen >= HEADER_LENGTH + dataLen)) {
      memcpy(codes[parameter], buffer + HEADER_LENGTH, dataLen);
      codes[parameter][dataLen - 1] = '\0';
      // return true;
    }
  }

  // Change devices address
  else if (opcode == CHANGE_DEVICE_ADDR) {

    if (dataLen > 0
        && dataLen < STRING_MAX_SIZE
        && (bufLen >= HEADER_LENGTH + dataLen)) {
      memcpy(deviceAddress, buffer + HEADER_LENGTH, dataLen);
      deviceAddress[dataLen - 1] = '\0';
      // return true;
    }
  }

  // return false;
}