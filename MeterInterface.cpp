#include "MeterInterface.h"

char dataBuf[TRANSMISSION_BUFFER_SIZE];
const char requestStart[] = "/?";
const char requestEnd[] = "!\r\n";
bool expectData = false;
int currentBaudIndex = INITIAL_BAUD_INDEX;

bool initMeterInterface() {
  pinMode(WB_IO2, OUTPUT);
  digitalWrite(WB_IO2, HIGH);
  RS485.setPins(RS485_TX_PIN, RS485_DE_PIN, RS485_RE_PIN);
  RS485.setTimeout(RS485_TIMEOUT);
  RS485.begin(ClassCMeterBaudRates[INITIAL_BAUD_INDEX], RS485_SERIAL_CONFIG);
  RS485.receive();
}

bool changeBaud(char newBaudIndex) {
  if (!(newBaudIndex < 7 && newBaudIndex >= 0)) {
    return false;
  }
  int newBaud = ClassCMeterBaudRates[newBaudIndex];

  // No change
  if (newBaud == ClassCMeterBaudRates[currentBaudIndex]) {
    return true;
  }

  // Set up new Baud
  currentBaudIndex = newBaudIndex;
  RS485.noReceive();
  RS485.end();
  RS485.begin(ClassCMeterBaudRates[currentBaudIndex], RS485_SERIAL_CONFIG);
  RS485.setTimeout(RS485_TIMEOUT);
  RS485.receive();
}

void sendBaudAck(char baudIndex) {
  char baudChar;
  if (!(baudIndex < 7 && baudIndex >= 0)) {
    baudChar = '0';
  } else {
    baudChar = '0' + baudIndex;
  }
  char ackBuf[] = { 0x06, '0', baudChar, '0', 0x0D, 0x0A , '\0' };
  RS485.beginTransmission();
  RS485.write(ackBuf, strlen(ackBuf));
  RS485.endTransmission();
  Serial.printf("Sent ack: %s for baud class %c.\r\n", ackBuf, baudChar);
}

void sendHandshake(char address[]) {
  // char buf1[] = {'/','?','8','6','9','0','3', '6', '3', '8','!',0x0D, 0x0A};
  expectData = false;
  RS485.beginTransmission();
  RS485.write(requestStart, strlen(requestStart));
  RS485.write(address, strlen(address));
  RS485.write(requestEnd, strlen(requestEnd));
  RS485.endTransmission();
  Serial.printf("Sent data: %s%s%s \r\n", requestStart, address, requestEnd);
}

bool processHandshakeResponse() {
  // Struct: /ISk5\2MT382-1000
  if (strlen(dataBuf) < 6 || dataBuf[0] != '/') {
    return false;
  }

  char baudIndex = 0;
  for (int i = 0; i < strlen(dataBuf); i++) {
    if (isDigit(dataBuf[i])) {
      baudIndex = dataBuf[i] - 30;
    }
  }

  sendBaudAck(baudIndex);
  if (changeBaud(baudIndex)) {
    Serial.println("Baud rate changed!");
  }
  return true;
}


void processRS485() {
  if (RS485.available()) {
    unsigned int dataLen = 0;
    if (expectData) {
      Packet packet;
byte sendBuf[100];
      dataLen = RS485.readBytesUntil('!', dataBuf, sizeof(dataBuf));
      dataBuf[dataLen] = '\0';
      for(int i = 0; i < dataLen; i++){
        Serial.printf("%c",dataBuf[i]);
      }
      Serial.printf("Bytes read: %u\r\n", dataLen);
      parseData(dataBuf, sizeof(dataBuf), &packet);
      int res = assemblePacket(sendBuf,100, packet);
      if(res > 0){
        Serial.printf("Packet size: %u\r\n",res);
        for(int i =0; i < res;i++){
          Serial.printf("0x%02hhx ",sendBuf[i]);
        }
      }
      expectData = false;
      changeBaud(0);
    } else {
      dataLen = RS485.readBytesUntil('!', dataBuf, sizeof(dataBuf));
      dataBuf[dataLen] = '\0';
      for(int i = 0; i < dataLen; i++){
        Serial.printf("%c",dataBuf[i]);
      }
      Serial.printf("Bytes read: %u\r\n", dataLen);
      if (processHandshakeResponse()) {
        expectData = true;
      }
      // RS485.flush();
    }
  }
}
