#include "MeterInterface.h"

char dataBuf[TRANSMISSION_BUFFER_SIZE];
const char requestStart[] = "/?";
const char requestEnd[] = "!\r\n";
bool expectData = false;
char currentBaudIndex = INITIAL_BAUD_INDEX;

void initMeterInterface() {
  pinMode(WB_IO2, OUTPUT);
  digitalWrite(WB_IO2, HIGH);
  RS485.setPins(RS485_TX_PIN, RS485_DE_PIN, RS485_RE_PIN);
  RS485.setTimeout(RS485_TIMEOUT);
  RS485.begin(ClassCMeterBaudRates[INITIAL_BAUD_INDEX], RS485_SERIAL_CONFIG);
  RS485.receive();
}

bool changeBaud(int newBaudIndex) {
  // Invalid baud
  if (!(newBaudIndex <= 6 && newBaudIndex >= 0)) {
    Serial.printf("Invalid baud index: %c.\r\n", newBaudIndex);
    return false;
  }

  // No change
  if (newBaudIndex == currentBaudIndex) {
    return false;
  }

  // Set up new Baud
  currentBaudIndex = newBaudIndex;
  int newBaud = ClassCMeterBaudRates[newBaudIndex];
  RS485.noReceive();
  RS485.end();
  RS485.begin(newBaud, RS485_SERIAL_CONFIG);
  RS485.setTimeout(RS485_TIMEOUT);
  RS485.receive();
  return true;
}

void sendBaudAck(int baudIndex) {
  char baudChar = '0' + static_cast<char>(baudIndex);
  char ackBuf[] = { 0x06, '0', baudChar, '0', 0x0D, 0x0A, '\0' };
  RS485.beginTransmission();
  RS485.write(ackBuf, strlen(ackBuf));
  RS485.endTransmission();
  Serial.printf("Sent ack: %s for baud class %c.\r\n", ackBuf, baudChar);
}

void sendHandshake(const char address[]) {
  // char buf1[] = {'/','?','8','6','9','0','3', '6', '3', '8','!',0x0D, 0x0A};
  expectData = false;
  RS485.beginTransmission();
  RS485.write(requestStart, strlen(requestStart));
  RS485.write(address, strlen(address));
  RS485.write(requestEnd, strlen(requestEnd));
  RS485.endTransmission();
  Serial.printf("Sent data: %s%s%s \r\n", requestStart, address, requestEnd);
}

bool isHandshakeResponse() {
  // Struct: /ISk5\2MT382-1000
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

  if (baudFound) {
    Serial.printf("Found baud index %c in reply.\r\n", result);
  }

  // ACK baud rate and change it
  if (baudFound && NEGOTIATE_BAUD) {
    Serial.printf("Will ACK for baud %d\r\n", ClassCMeterBaudRates[result]);
    sendBaudAck(result);
    changeBaud(result);
  }

  // ACK and keep original baud
  else {
    sendBaudAck(INITIAL_BAUD_INDEX);
  }
  return true;
}

// Process incoming RS485 transmissions in main loop
void processRS485() {
  int availableBytes = RS485.available();
  if (availableBytes) {
    unsigned int dataLen = 0;

    // Handshake was successful and now we expect a data packet
    if (expectData) {
      Packet packet;
      dataLen = RS485.readBytesUntil('!', dataBuf, sizeof(dataBuf));
      dataBuf[dataLen] = '\0';
      for (unsigned int i = 0; i < dataLen; i++) {
        Serial.printf("%c", dataBuf[i]);
      }
      Serial.printf("Bytes read: %u\r\n", dataLen);

      // Parse IEC-62056-21 ASCII packet
      if (parseData(dataBuf, sizeof(dataBuf), &packet)) {

        // Assemble and send packet
        byte sendBuf[LORAWAN_APP_DATA_BUFF_SIZE];
        int res = assemblePacket(sendBuf, 100, packet);
        int sendError = send_lora_frame(sendBuf, res);

        if (res && !sendError) {
          Serial.println("Packet sent successfully!");
          Serial.printf("Packet size: %u\r\n", res);
          for (int i = 0; i < res; i++) {
            Serial.printf("0x%02hhx ", sendBuf[i]);
          }
        } else {
          Serial.printf("Error sending packet. E:%d, pktlen: %u\r\n", sendError, res);
        }
      } else {
        Serial.println("Error parsing the data recieved.");
      }
      RS485.flush();
      changeBaud(INITIAL_BAUD_INDEX);
      expectData = false;
    }

    // Not expecting data, probably expecting the handshake response
    else {
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
