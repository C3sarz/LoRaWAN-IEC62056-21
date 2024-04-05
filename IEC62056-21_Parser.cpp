#include <cstring>
#include "IEC62056-21_Parser.h"

bool isQueriedCode(char* code, int* codeIndex) {
  for (int i = 0; i < CODES_LIMIT && loadedCodes[i] != NULL; i++) {
    if (strcmp(code, loadedCodes[i]) == 0) {
      *(codeIndex) = i;
      return true;
    }
  }
  return false;
}


bool parseDataString(char dataStr[], int len, ParsedDataObject* dataPtr) {

  char* idStringPtr;
  char* dataStartPtr;
  char* dataBlockEndPtr;

  idStringPtr = strchr(dataStr, ':');
  dataStartPtr = strchr(dataStr, '(');
  dataBlockEndPtr = strchr(dataStr, ')');
  if (idStringPtr == NULL || dataStartPtr == NULL || dataBlockEndPtr == NULL) {
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
#if LOGLEVEL >= 1
    Serial.println("================== CODE ===================");
    Serial.printf("Code: %s\r\n", idStringPtr);
    Serial.printf("Data: %s\r\n", dataStartPtr);
#endif
    saveNumericalValue(dataStartPtr, valueIndex, dataPtr);
    return true;
  }
  return false;
}

// Parses the IEC 62056-21 data block DESTRUCTIVELY, and saves relevant values if found.
bool parseDataBlock(byte buffer[], int len, ParsedDataObject* dataPtr) {

  // String processing pointers
  char* nextItem;
  char* currentItem = (char*)buffer;

  // Counters
  unsigned int count = 0;
  unsigned int foundCount = 0;

  // Set starting position
  nextItem = strchr(currentItem, '\n');
  if (nextItem == NULL) {
    return false;
  }

  // Iterate throughout data block
  while (nextItem != NULL) {
    nextItem[0] = '\0';
    count++;
    if (parseDataString(currentItem, strlen(currentItem), dataPtr)) {
#if LOGLEVEL >= 2
      Serial.printf("-------------------\r\nLine: %s\r\n", currentItem);
#endif
      foundCount++;
    }

    currentItem = nextItem + 1;
    nextItem = strchr(nextItem + 1, '\n');
  }
  // DEBUG
  if (foundCount > 0) {
    Serial.printf("Finished parsing %u lines.\r\n", count);
    for (int i = 0; i < CODES_LIMIT; i++) {
      Serial.printf("CHANNEL #%d: %d, Decimals: %u\r\n", i + 1, dataPtr->values[i], dataPtr->decimalPoints[i]);
    }
  }

  return foundCount > 0;
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

/* Removes decimal point from value, transforming it into an INT, and saves it for TX.
   Also saves the decimal place count, and marks the item as present in the mask.
*/
void saveNumericalValue(char* valueStr, int position, ParsedDataObject* dataPtr) {
  char* decimalCharPtr = strchr(valueStr, '.');
  if (decimalCharPtr != NULL) {
    for (unsigned int i = 0; i < strlen(decimalCharPtr); i++) {
      decimalCharPtr[i] = decimalCharPtr[i + 1];
    }
    dataPtr->decimalPoints[position] = strlen(decimalCharPtr);
  } else {
    dataPtr->decimalPoints[position] = 0;
  }
  Serial.printf("Mask for %d: %x\r\n", position, getPositionMask(position));
  dataPtr->itemPresentMask |= getPositionMask(position);
  dataPtr->values[position] = atol(valueStr);
  Serial.printf("StrVal: %s, Val: %d\r\n", valueStr, atol(valueStr));
  return;
}
