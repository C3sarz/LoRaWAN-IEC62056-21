#include "MeterInterface.h"

char dataBuf[TRANSMISSION_BUFFER_SIZE];
const char requestStart[] = "/?";
const char requestEnd[] = "!\r\n";
bool expectData = false;

void initMeterInterface() {
  pinMode(WB_IO2, OUTPUT);
  digitalWrite(WB_IO2, HIGH);
  RS485.setPins(RS485_TX_PIN, RS485_DE_PIN, RS485_RE_PIN);
  RS485.setTimeout(RS485_TIMEOUT);
  RS485.begin(ClassCMeterBaudRates[currentBaudIndex], RS485_SERIAL_CONFIG);
  RS485.receive();
}

// Change RS485 baud rate
bool changeBaud(int newBaudIndex) {
  // Invalid baud
  if (!(newBaudIndex <= 6 && newBaudIndex >= 0)) {
    Serial.printf("Invalid baud index: %c.\r\n", newBaudIndex);
    return false;
  }

  // No change
  if (newBaudIndex == currentBaudIndex) {
    Serial.println("No change in baud");
    return false;
  }

  // Set up new baud rate
  currentBaudIndex = newBaudIndex;
  int newBaud = ClassCMeterBaudRates[newBaudIndex];
  RS485.noReceive();
  RS485.end();
  RS485.begin(newBaud, RS485_SERIAL_CONFIG);
  RS485.setTimeout(RS485_TIMEOUT);
  RS485.receive();
  Serial.printf("Baud %d\r\n", newBaud);
  return true;
}

// Send ACK to meter to begin data transfer
void sendAck(int baudIndex) {
  char baudChar = '0' + static_cast<char>(baudIndex);
  char ackBuf[] = { 0x06, '0', baudChar, '0', 0x0D, 0x0A, '\0' };
  RS485.beginTransmission();
  RS485.write(ackBuf, strlen(ackBuf));
  RS485.endTransmission();
  Serial.printf("Sent ack: %s for baud class %c.\r\n", ackBuf, baudChar);
}

// Initiate communication with a meter with a handshake
void sendQuery(const char address[]) {

  expectData = false;
  RS485.beginTransmission();
  RS485.write(requestStart, strlen(requestStart));
  RS485.write(address, strlen(address));
  RS485.write(requestEnd, strlen(requestEnd));
  RS485.endTransmission();
  Serial.printf("Sent data: %s%s%s \r\n", requestStart, address, requestEnd);
}

// Check if recieved packet is the handshake response and act upon result
bool isHandshakeResponse() {
  // Struct: /ISk5\2MT382-1000
  ///         ISk5ME172-0000
  char* idPtr = strchr(dataBuf, '/');
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
      baudFound = true;
    }
  }

  // ACK baud rate and change it
  if (baudFound && NEGOTIATE_BAUD) {
    Serial.printf("Will ACK for baud %d\r\n", ClassCMeterBaudRates[result]);
    sendAck(result);

    // Message gets corrupted without the delay
    delay(100);
    changeBaud(result);
  }

  // ACK and keep original baud
  else {
    sendAck(DEFAULT_BAUD_INDEX);
  }

  return true;
}

// Process incoming RS485 transmissions in main loop
void processRS485() {
  static unsigned int statusCounter = 0;
  int availableBytes = RS485.available();

  if (availableBytes) {
    unsigned int dataLen = 0;

    // Handshake was successful and now we expect a data packet
    if (expectData) {
      ParsedDataObject dataObj;
      dataLen = RS485.readBytesUntil('!', dataBuf, sizeof(dataBuf));
      dataBuf[dataLen] = '\0';
      for (unsigned int i = 0; i < dataLen; i++) {
        Serial.printf("%c", dataBuf[i]);
      }
      Serial.printf("Bytes read: %u\r\n", dataLen);

      // Parse IEC-62056-21 ASCII data
      if (parseDataBlockNew(dataBuf, sizeof(dataBuf), &dataObj)) {

        // Assemble and send LoRaWAN packet
        byte sendBuf[LORAWAN_APP_DATA_BUFF_SIZE];

        int result = 0;
        if (++statusCounter >= STATUSPACKETCOUNT) {
          statusCounter = 0;
          result = assembleStatusPacket(sendBuf, dataObj);
        } else {

          result = assembleDataPacket(sendBuf, dataObj);
        }


        int sendError = -1;
        if (linkCheckCount >= CONFIRMED_COUNT) {
          sendError = send_lora_frame(sendBuf, result, true);
        } else {
          sendError = send_lora_frame(sendBuf, result, false);
        }

        if (result && !sendError) {
          digitalWrite(PIN_LED1, 0);
          Serial.println("Packet sent successfully!");
          Serial.printf("Packet size: %u\r\n", result);
          for (int i = 0; i < result; i++) {
            Serial.printf("0x%02hhx ", sendBuf[i]);
          }
        } else {
          Serial.printf("Error sending packet. E:%d, pktlen: %u\r\n", sendError, result);
        }
      } else {
        Serial.println("Error parsing the data recieved.");
      }
      RS485.flush();
      expectData = false;
    }

    // Not expecting data, probably expecting the handshake response
    else {
      // dataLen = RS485.readBytesUntil(0x0D, dataBuf, sizeof(dataBuf));
      dataLen = RS485.readBytes(dataBuf, sizeof(dataBuf));

      dataBuf[dataLen] = '\0';
      for (unsigned int i = 0; i < dataLen; i++) {
        Serial.printf("%c", dataBuf[i]);
      }
      Serial.printf("Bytes read: %u\r\n", dataLen);
      if (isHandshakeResponse()) {
        expectData = true;
        RS485.flush();
      } else {
        expectData = false;
      }
    }
  }
}

byte assembleInitPacket(byte* dataPtr) {
  byte dataLen = 0;
  dataPtr[dataLen++] = INIT;

  uint16_t vBat = getBatteryInt();
  dataPtr[dataLen++] = static_cast<byte>((vBat & 0xFF00) >> 8);
  dataPtr[dataLen++] = static_cast<byte>(vBat & 0x00FF);

  return dataLen;
}

byte assembleErrorPacket(Error_Type error, byte* dataPtr) {
  byte dataLen = 0;
  dataPtr[dataLen++] = ERROR;

  uint16_t vBat = getBatteryInt();
  dataPtr[dataLen++] = static_cast<byte>((vBat & 0xFF00) >> 8);
  dataPtr[dataLen++] = static_cast<byte>(vBat & 0x00FF);
  dataPtr[dataLen++] = error;

  return dataLen;
}

byte assembleStatusPacket(byte* resultBuffer, ParsedDataObject data) {
  byte packetLen = 0;

  if (!(data.itemPresentMask)) {
    return 0;
  }

  // STATUS Command
  resultBuffer[packetLen++] = STATUS;

  uint16_t vBat = getBatteryInt();
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
