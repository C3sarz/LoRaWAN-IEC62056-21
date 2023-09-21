#include <Arduino.h>
#include <string.h>
#define CODES_LIMIT 16

class Packet {
  public:
    int32_t values[CODES_LIMIT];
    byte decimalPoints[CODES_LIMIT];
    uint16_t itemPresentMask = 0;
    byte codesCount = 0;

    Packet(void){
      memset(values,0,CODES_LIMIT*sizeof(values[0]));
      memset(decimalPoints,0,CODES_LIMIT*sizeof(byte));
    }
};

bool parseData(char buffer[], int len, Packet* packetPtr);
byte assemblePacket(byte * resultBuffer, int maxLen, Packet packet);
bool isQueriedCode(char* code, int* codeIndex);
void saveNumericalValue(char* valueStr, int position, Packet* packetPtr);
uint32_t getDecimalCountMask(byte decimals[]);
uint16_t getPositionMask(uint16_t position);
unsigned int countCodes();
