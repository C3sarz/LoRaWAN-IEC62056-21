#ifndef _PARSER_
#define _PARSER_

#include "Storage.h"
#include <string.h>

class ParsedDataObject {
public:
  int32_t values[CODES_LIMIT];
  byte decimalPoints[CODES_LIMIT];
  uint16_t itemPresentMask = 0;
  byte codesCount = 0;

  ParsedDataObject(void) {
    memset(values, 0, CODES_LIMIT * sizeof(values[0]));
    memset(decimalPoints, 0, CODES_LIMIT * sizeof(byte));
  }
};

extern CodeString codes[CODES_LIMIT + 1];

bool parseDataBlockNew(char buffer[], int len, ParsedDataObject* dataPtr);
bool parseDataBlockOld(char buffer[], int len, ParsedDataObject* dataPtr);
bool parseDataString(char dataStr[], int len, ParsedDataObject* dataPtr);
bool isQueriedCode(char* code, int* codeIndex);
void saveNumericalValue(char* valueStr, int position, ParsedDataObject* dataPtr);
uint32_t getDecimalCountMask(byte decimals[]);
uint16_t getPositionMask(uint16_t position);

#endif