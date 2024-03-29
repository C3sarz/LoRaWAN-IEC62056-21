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
    return false;
  }

  // Set up new baud rate
  currentBaudIndex = newBaudIndex;
  int newBaud = ClassCMeterBaudRates[newBaudIndex];
  Serial.printf("===Available: %d\r\n", RS485.availableForWrite());
  RS485.noReceive();
  RS485.end();
  RS485.begin(newBaud, RS485_SERIAL_CONFIG);
  RS485.setTimeout(RS485_TIMEOUT);
  RS485.receive();
  return true;
}

// Send ACK to meter to begin data transfer
void sendBaudAck(int baudIndex) {
  char baudChar = '0' + static_cast<char>(baudIndex);
  char ackBuf[] = { 0x06, '0', baudChar, '0', 0x0D, 0x0A, '\0' };
  RS485.beginTransmission();
  RS485.write(ackBuf, strlen(ackBuf));
  RS485.endTransmission();
  Serial.printf("Sent ack: %s for baud class %c.\r\n", ackBuf, baudChar);
}

// Initiate communication with a meter with a handshake
void sendHandshake(const char address[]) {
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
    sendBaudAck(result);

    // Message gets corrupted without the delay
    delay(100);
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

        int sendError = -1;
        if (linkCheckCount >= CONFIRMED_COUNT) {
          sendError = send_lora_frame(sendBuf, res, true);
        } else {
          sendError = send_lora_frame(sendBuf, res, false);
        }

        if (res && !sendError) {
          digitalWrite(PIN_LED1, 0);
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
      if (changeBaud(INITIAL_BAUD_INDEX)) {
        Serial.printf("Reset baud to %d.\r\n", ClassCMeterBaudRates[INITIAL_BAUD_INDEX]);
      }
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
