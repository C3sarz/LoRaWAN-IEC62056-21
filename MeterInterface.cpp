#include "MeterInterface.h"

byte dataBuf[UART_BUFFER_SIZE];
const char REQUESTSTART[] = "/?";
const char REQUESTEND[] = "!\r\n";
byte queryAttempts = MAX_QUERY_ATTEMPTS - 1;

// Set up RS485 interface
void initMeterInterface(void) {

  // Setup RS485 pins
  pinMode(RS485_DE_PIN, OUTPUT);
  pinMode(RS485_RE_PIN, OUTPUT);

  pinMode(WB_IO2, OUTPUT);
  digitalWrite(RS485_RE_PIN, HIGH);
  digitalWrite(RS485_DE_PIN, LOW);
  digitalWrite(WB_IO2, HIGH);
  Serial1.setTimeout(RS485_TIMEOUT);
  Serial1.begin(ClassCMeterBaudRates[currentBaudIndex], RS485_SERIAL_CONFIG);
}

// Change RS485 baud rate
bool changeBaudRS485(int newBaudIndex) {
  // Invalid baud
  if (!(newBaudIndex <= 6 && newBaudIndex >= 0)) {
    Serial.printf("Invalid baud index: %c.\r\n", newBaudIndex);
    return false;
  }

  // No change
  if (newBaudIndex == currentBaudIndex) {
    // Serial.println("No change in baud");
    return false;
  }

  // Set up new baud rate
  currentBaudIndex = newBaudIndex;
  int newBaud = ClassCMeterBaudRates[newBaudIndex];
  digitalWrite(WB_IO2, LOW);
  digitalWrite(RS485_RE_PIN, LOW);
  Serial1.flush();
  Serial1.end();
  Serial1.begin(newBaud, RS485_SERIAL_CONFIG);
  Serial1.setTimeout(RS485_TIMEOUT);
  digitalWrite(WB_IO2, HIGH);
  digitalWrite(RS485_RE_PIN, HIGH);
  delay(100);
#if LOGLEVEL >= 2
  Serial.printf("New Baud %d\r\n", newBaud);
#endif
  return true;
}

size_t writeRS485(byte* buffer, size_t bufferLen) {
  size_t result = 0;
  digitalWrite(RS485_DE_PIN, HIGH);
  result = Serial1.write(buffer, bufferLen);
  digitalWrite(RS485_DE_PIN, LOW);
  return result;
}

size_t readRS485(byte* buffer, size_t bufferLen) {
  // Check if RS485 data in buffer
  if (Serial1.available() > 0) {
    return Serial1.readBytesUntil('!', buffer, bufferLen);
  }
  return 0;
}

// Send ACK to meter to begin data transfer
void sendAck(int baudIndex) {
  char baudChar = '0' + static_cast<char>(baudIndex);
  byte ackBuf[] = { 0x06, '0', baudChar, '0', 0x0D, 0x0A, '\0' };
  writeRS485(ackBuf, sizeof(ackBuf));
#if LOGLEVEL >= 2
  Serial.printf("Sent ack: %s for baud class %c.\r\n", ackBuf, baudChar);
#endif
}

// Initiate communication with a meter with a handshake
void sendQuery(const char address[]) {

  if (queryAttempts >= MAX_QUERY_ATTEMPTS) {
    byte packet[10];
    byte packetLen = assembleErrorPacket(QUERY_TIMEOUT, queryAttempts, packet);
    queryAttempts = 0;
    sendUplink(packet, packetLen, true);
  }

  // Set baud to default
  changeBaudRS485(DEFAULT_BAUD_INDEX);

  // Assemble TX buffer
  unsigned int sentBytes = 0;
  sentBytes += writeRS485((byte*)REQUESTSTART, strlen(REQUESTSTART));
  sentBytes += writeRS485((byte*)address, strlen(address));
  sentBytes += writeRS485((byte*)REQUESTEND, strlen(REQUESTEND));
  queryAttempts++;
  Serial.printf("Send query #%u: %s%s%s \r\n", queryAttempts, REQUESTSTART, address, REQUESTEND);
}

// Check if recieved packet is the handshake response and act upon result
bool isHandshakeResponse(int* newBaud) {
  // Struct: /ISk5\2MT382-1000
  //         ISk5ME172-0000
  char* idPtr = strchr((char*)dataBuf, '/');
  int result;
  bool baudFound = false;

  // Not ID response packet
  if (idPtr == NULL || strlen(idPtr) < 5) {
    return false;
  }

  // Check baud char
  if (isDigit(idPtr[4])) {
    result = idPtr[4] - '0';
    if (result <= 6 && result >= 0) {
      *newBaud = result;
      baudFound = true;
    }
  }
  return baudFound;
}

// Process incoming RS485 transmissions in main loop
void processRS485() {
  static unsigned int statusCounter = 0;
  ParsedDataObject dataObj;
  int availableBytes = readRS485(dataBuf, sizeof(dataBuf));

  // Check if RS485 in buffer
  if (availableBytes) {
    unsigned int dataLen = availableBytes;
    dataBuf[dataLen] = '\0';
#if LOGLEVEL >= 2
    for (unsigned int i = 0; i < dataLen; i++) {
      Serial.printf("%c", dataBuf[i]);
    }
    Serial.printf("\r\n=======\r\nBytes read: %u\r\n", dataLen);
#endif

    // Check for HANDSHAKE RESPONSE
    int newBaud = DEFAULT_BAUD_INDEX;
    if (isHandshakeResponse(&newBaud)) {

      // ACK and change baudrate
      if (NEGOTIATE_BAUD) {
        Serial.printf("Will ACK for baud %d\r\n", ClassCMeterBaudRates[newBaud]);
        sendAck(newBaud);
        delay(100);
        changeBaudRS485(newBaud);
      }

      // ACK and keep original baud
      else {
        sendAck(DEFAULT_BAUD_INDEX);
      }
    }

    // Try parse IEC-62056-21 ASCII data
    else if (parseDataBlock(dataBuf, sizeof(dataBuf), &dataObj)) {

      // Assemble and send LoRaWAN packet
      byte sendBuf[LORAWAN_APP_DATA_BUFF_SIZE];
      int packetLen = 0;
      if (++statusCounter >= STATUSPACKETCOUNT) {
        statusCounter = 0;
        packetLen = assembleStatusPacket(sendBuf, dataObj);
      } else {
        packetLen = assembleDataPacket(sendBuf, dataObj);
      }

      Serial.printf("Current DR: %u\r\n",getADRDatarate());

      // Send packet
      byte sendError = sendUplink(sendBuf, packetLen, linkCheckCount >= CONFIRMED_COUNT);
      if (packetLen && !sendError) {
        digitalWrite(PIN_LED1, 0);
        queryAttempts = 0;
        Serial.println("Packet sent successfully!");
        Serial.printf("Packet size: %u\r\n", packetLen);
        for (int i = 0; i < packetLen; i++) {
          Serial.printf("0x%02hhx ", sendBuf[i]);
        }
      } else {
        Serial.printf("Error sending packet. E:%d, pktlen: %u\r\n", sendError, packetLen);
      }
    } else {
#if LOGLEVEL >= 0
      Serial.println("Error parsing the data recieved.");
#endif
    }
  }
}

// Assemble status packet that is sent every few data packets.
byte assembleStatusPacket(byte* resultBuffer, ParsedDataObject data) {
  byte packetLen = 0;

  if (!(data.itemPresentMask)) {
    return 0;
  }

  // STATUS Command
  resultBuffer[packetLen++] = STATUS;

  // Get peripheral data
  uint16_t vBat = getVBatInt();
  resultBuffer[packetLen++] = static_cast<byte>((vBat & 0xFF00) >> 8);
  resultBuffer[packetLen++] = static_cast<byte>(vBat & 0x00FF);

  // Add present values indicator to packet.
  // Copy byte for byte reversing endianess.
  for (unsigned int i = 0; i < sizeof(data.itemPresentMask); i++) {
    resultBuffer[packetLen++] = (data.itemPresentMask >> 8 * (sizeof(data.itemPresentMask) - 1 - i)) & 0xFF;
  }

  // Add decimals count mask.
  uint32_t decimals = getDecimalCountMask(data.decimalPoints);
  // Copy byte for byte reversing endianess.
  for (unsigned int i = 0; i < sizeof(decimals); i++) {
    resultBuffer[packetLen++] = (decimals >> 8 * (sizeof(decimals) - 1 - i)) & 0xFF;
  }

  // Add values (if found).
  uint16_t presentValues = data.itemPresentMask;
  for (int item = 0; item < CODES_LIMIT; item++) {
    if (presentValues & 0x01) {
      // Copy byte for byte reversing endianess.
      for (unsigned int i = 0; i < sizeof(data.values[0]); i++) {
        resultBuffer[packetLen++] = (data.values[item] >> 8 * (sizeof(data.values[0]) - 1 - i)) & 0xFF;
      }
    }
    presentValues = presentValues >> 1;
  }
  return packetLen;
}

// Assemble uplink packet with meter data.
byte assembleDataPacket(byte* resultBuffer, ParsedDataObject data) {
  byte packetLen = 0;

  if (!(data.itemPresentMask)) {
    return 0;
  }

  // DATA Command
  resultBuffer[packetLen++] = DATA;

  // Add present values indicator to packet.
  // Copy byte for byte reversing endianess.
  for (unsigned int i = 0; i < sizeof(data.itemPresentMask); i++) {
    resultBuffer[packetLen++] = (data.itemPresentMask >> 8 * (sizeof(data.itemPresentMask) - 1 - i)) & 0xFF;
  }

  // Add decimals count mask.
  uint32_t decimals = getDecimalCountMask(data.decimalPoints);
  // Copy byte for byte reversing endianess.
  for (unsigned int i = 0; i < sizeof(decimals); i++) {
    resultBuffer[packetLen++] = (decimals >> 8 * (sizeof(decimals) - 1 - i)) & 0xFF;
  }

  // Add values (if found).
  uint16_t presentValues = data.itemPresentMask;
  for (int item = 0; item < CODES_LIMIT; item++) {
    if (presentValues & 0x01) {
      // Copy byte for byte reversing endianess.
      for (unsigned int i = 0; i < sizeof(data.values[0]); i++) {
        resultBuffer[packetLen++] = (data.values[item] >> 8 * (sizeof(data.values[0]) - 1 - i)) & 0xFF;
      }
    }
    presentValues = presentValues >> 1;
  }
  return packetLen;
}
