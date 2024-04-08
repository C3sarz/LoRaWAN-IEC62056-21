#include "LoRaWAN_Interface.h"
#include "LoRaWAN_Implementation.h"
#include "StorageInterface.h"
#include "Peripherals.h"

static LoRaWAN_Callbacks callbacks ={
  onJoinConfirmedCallback,
  onJoinFailedCallback,
  onUnconfirmedSentCallback,
  onConfirmedSentCallback,
  onDownlinkRecievedCallback,
};

void onDownlinkRecievedCallback(byte* recievedData, unsigned int dataLen, byte fPort, int rssi, byte snr) {

Serial.printf("DL at FPort %u: RSSI: %d | SNR: %f \r\n",fPort, rssi, snr);
if (processDownlinkPacket(recievedData, dataLen)) {
    Serial.println("Downlink processing succcessful");
  } else {
    Serial.println("Unknown or invalid downlink command");
  }
}

byte sendUplink(byte* sendBuffer, size_t bufferLen, bool confirmed) {
  return ISendLoRaWAN(sendBuffer, bufferLen, confirmed);
}

bool initLoRaWAN(void) {
  if (!ISetupLoRaWAN(&callbacks)) {
    return false;
  }
  tryJoin();
  return true;
}

void tryJoin(void) {
  Serial.println("Joining...");
  IJoinNetwork();
}

void onJoinConfirmedCallback(void) {

    Serial.println("OTAA Mode, Network Joined!");
    digitalWrite(PIN_LED2, 0);
      delay(10000);
      byte data[10];
      byte len = assembleInitPacket(data);
      ISendLoRaWAN(data, len, false);
}

void onJoinFailedCallback(void) {
  static byte failCount = 0;
  Serial.println("OTAA join failed!");
  if(failCount++  > 10){
    setReboot = true;
  }
  delay(30000);
  tryJoin();
}

void onUnconfirmedSentCallback(void) {
  Serial.println("TX finished");
  linkCheckCount++;
}
void onConfirmedSentCallback(bool result) {
  Serial.printf("Confirmed TX %s\n", result ? "success" : "failed");
  static byte failCount = 0;
  if (!result) {
    failCount++;
    if (failCount > 3) {
      setReboot = true;
    }
  } else {
    linkCheckCount = 0;
    failCount = 0;
  }
}

// Assemble packet that is sent when the device joins the network.
byte assembleInitPacket(byte* dataPtr) {
  byte dataLen = 0;
  dataPtr[dataLen++] = INIT;

  uint16_t vBat = getVBatInt();
  dataPtr[dataLen++] = static_cast<byte>((vBat & 0xFF00) >> 8);
  dataPtr[dataLen++] = static_cast<byte>(vBat & 0x00FF);

  return dataLen;
}

// Assemble packet that is sent when we get an error.
byte assembleErrorPacket(byte error, byte* dataPtr) {
  byte dataLen = 0;
  dataPtr[dataLen++] = ERROR;

  uint16_t vBat = getVBatInt();
  dataPtr[dataLen++] = static_cast<byte>((vBat & 0xFF00) >> 8);
  dataPtr[dataLen++] = static_cast<byte>(vBat & 0x00FF);
  dataPtr[dataLen++] = error;

  return dataLen;
}
