#ifndef _METER_HANDLER_
#define _METER_HANDLER_

#include "StorageInterface.h"
#include "LoRaWAN_Interface.h"
#include "IEC62056-21_Parser.h"
#include "Peripherals.h"
#include <string.h>

// RS485 PINS
#define RS485_TX_PIN 0
#define RS485_DE_PIN 6
#define RS485_RE_PIN 1

#define NEGOTIATE_BAUD 1
#define RS485_SERIAL_CONFIG SERIAL_7E1
#define RS485_TIMEOUT 200
#define UART_BUFFER_SIZE 5000
#define STATUSPACKETCOUNT 10

// IEC 62056-21 Mode C serial baud rates
const int ClassCMeterBaudRates[]{
  300,
  600,
  1200,
  2400,
  4800,
  9600,
  19200
};

enum Meter_Error_Type {
  QUERY_TIMEOUT = 1,
  ERROR_UNSPECIFIED,
};

extern int currentBaudIndex;

void initMeterInterface();
size_t readRS485(byte* buffer, size_t bufferLen);
size_t writeRS485(byte* buffer, size_t bufferLen);
void processRS485();
byte assembleDataPacket(byte* resultBuffer, ParsedDataObject data);
byte assembleStatusPacket(byte* resultBuffer, ParsedDataObject data);
bool changeBaudRS485(int newBaudIndex);
void sendAck(int baudIndex);
void sendQuery(const char address[]);
bool isHandshakeResponse(int* newBaud);

#endif