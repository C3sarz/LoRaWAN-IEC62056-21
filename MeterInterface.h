#include "IEC62056-21_Parser.h"
#include <ArduinoRS485.h> //Click here to get the library: http://librarymanager/All#ArduinoRS485
#include "LoRaWAN_Handler.h"
#include "string.h"

#define RS485_TX_PIN 0
#define RS485_DE_PIN 6
#define RS485_RE_PIN 1

#define INITIAL_BAUD_INDEX 5
#define NEGOTIATE_BAUD 1
#define RS485_SERIAL_CONFIG SERIAL_7E1
#define RS485_TIMEOUT 200
#define TRANSMISSION_BUFFER_SIZE 5000

// IEC 62056-21 Mode C Rates
const int ClassCMeterBaudRates[] {
  300,
  600,
  1200,
  2400,
  4800,
  9600,
  19200
};

const char NOADDRESS[] = "0";

void initMeterInterface();
void processRS485();
bool changeBaud(int newBaudIndex);
void sendBaudAck(int baudIndex);
void sendHandshake(const char address[]);
bool isHandshakeResponse();