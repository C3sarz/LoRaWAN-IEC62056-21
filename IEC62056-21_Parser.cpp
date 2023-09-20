#include "IEC62056-21_Parser.h"

// export const char* codes[CODES_LIMIT + 1] = {
//   "32.7.0",
//   "52.7.0",
//   "72.7.0",
//   "0.9.2*255",
//   "15.8.0*02",
//   NULL
// };

export const char* codes[] = {
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
  "15.4.0",
  "15.8.0",
  "15.6.0",
  "13.5.0",
  NULL
};

byte assemblePacket(byte* resultBuffer, int maxLen, Packet packet) {
  int packetLen = 0;

  if(!(packet.itemPresentMask)){
    return 0;
  }

  // Add present values indicator to packet.
  // Copy byte for byte reversing endianess.
  for (int i = 0; i < sizeof(packet.itemPresentMask); i++) {
    resultBuffer[packetLen++] = (packet.itemPresentMask >> 8 * (sizeof(packet.itemPresentMask) - 1 - i)) & 0xFF;
  }

  // Add decimals count mask.
  uint32_t decimals = getDecimalCountMask(packet.decimalPoints);
  // Copy byte for byte reversing endianess.
  for (int i = 0; i < sizeof(decimals); i++) {
    resultBuffer[packetLen++] = (decimals >> 8 * (sizeof(decimals) - 1 - i)) & 0xFF;
  }

  // Add values (if found).
  uint16_t presentValues = packet.itemPresentMask;
  for (int item = 0; item < CODES_LIMIT; item++) {
    if (presentValues & 0x01) {
      // Copy byte for byte reversing endianess.
      for (int i = 0; i < sizeof(packet.values[0]); i++) {
        resultBuffer[packetLen++] = (packet.values[item] >> 8 * (sizeof(packet.values[0]) - 1 - i)) & 0xFF;
      }
    }
    presentValues = presentValues >> 1;
  }
  return packetLen;
}

bool isQueriedCode(char* code, int* codeIndex) {
  for (int i = 0; i < CODES_LIMIT && codes[i] != NULL; i++) {
    if (strcmp(code, codes[i]) == 0) {
      *(codeIndex) = i;
      return true;
    }
  }
  return false;
}

bool parseData(char buffer[], int len, Packet* packetPtr) {
  unsigned int count = 0;

  // String processing pointers
  char* nextItem;
  char* currentItem = buffer;
  char* idStringPtr;
  char* dataStartPtr;
  char* dataBlockEndPtr;

  nextItem = strchr(buffer, '\n');
  if (nextItem == NULL) {
    return false;
  }
  Serial.println("Parsing test..");

  while (nextItem != NULL) {
    nextItem[0] = '\0';
    count++;

    Serial.printf("-------------------\r\nLine: %s\r\n", currentItem);

    idStringPtr = strchr(currentItem, ':');
    dataStartPtr = strchr(currentItem, '(');
    dataBlockEndPtr = strchr(currentItem, ')');
    if (idStringPtr == NULL || dataStartPtr == NULL) {
      return false;
    }

    // Check for units separator (value*units)
    char* valueEndPtr = strchr(dataStartPtr, '*');
    if (valueEndPtr != NULL) {
      dataBlockEndPtr = valueEndPtr;
    }

    // Modify string
    dataStartPtr[0] = '\0';
    dataBlockEndPtr[0] = '\0';
    idStringPtr++;
    dataStartPtr++;

    int valueIndex;
    if (isQueriedCode(idStringPtr, &valueIndex)) {
      Serial.println("================== CODE FOUND ===================");
      Serial.printf("Code: %s\r\n", idStringPtr);
      Serial.printf("Data: %s\r\n", dataStartPtr);
      saveNumericalValue(dataStartPtr, valueIndex, packetPtr);
    }

    currentItem = nextItem + 1;
    nextItem = strchr(nextItem + 1, '\n');
  }
  // DEBUG
  Serial.printf("Finished parsing %u lines.\r\n", count);
  for (int i = 0; i < CODES_LIMIT; i++) {
    Serial.printf("CHANNEL #%d: %d, Decimals: %u\r\n", i+1, packetPtr->values[i], packetPtr->decimalPoints[i]);
  }


  // No codes found
  if (packetPtr->itemPresentMask <= 0) {
    Serial.println("No codes found!");
    return false;
  }

  // Data saved in saveNumericalValue and returned through the packet pointer.
  return true;
}

uint16_t getPositionMask(int position) {
  uint16_t value = 0x01;
  for (int i = 0; i < (position % CODES_LIMIT); i++) {
    value = value << 1;
  }
  return value;
}

// Create mask where each 2 BITS represent the amount of decimal positions for a value.
uint32_t getDecimalCountMask(byte decimals[]) {
  uint32_t result = 0;
  for (int i = 0; i < CODES_LIMIT; i++) {
    uint32_t value = decimals[i];
    value = value << (2 * i);
    result |= value;
  }
  return result;
}

void saveNumericalValue(char* valueStr, int position, Packet* packetPtr) {
  char* decimalCharPtr = strchr(valueStr, '.');
  if (decimalCharPtr != NULL) {
    for (int i = 0; i < strlen(decimalCharPtr); i++) {
      decimalCharPtr[i] = decimalCharPtr[i + 1];
    }
    packetPtr->decimalPoints[position] = strlen(decimalCharPtr);
  } else {
    packetPtr->decimalPoints[position] = 0;
  }
  Serial.printf("Mask for %d: %x\r\n", position, getPositionMask(position));
  packetPtr->itemPresentMask |= getPositionMask(position);
  packetPtr->values[position] = atol(valueStr);
  Serial.printf("StrVal: %s, Val: %d\r\n", valueStr, atol(valueStr));
  return;
}
