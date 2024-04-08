#ifndef _LORAWAN_INTERFACE_
#define _LORAWAN_INTERFACE_

#include "config.h"

// Data packet type
enum Packet_Type {
  INIT = 0x0A,
  STATUS,
  ERROR,
  DATA,
};

// LoRaWAN Keys
extern uint8_t nodeDeviceEUI[8];
extern uint8_t nodeAppEUI[8];
extern uint8_t nodeAppKey[16];
extern volatile bool setReboot;

bool initLoRaWAN(void);
void tryJoin(void);
void onJoinConfirmedCallback(void);
void onJoinFailedCallback(void);
void onUnconfirmedSentCallback(void);
void onConfirmedSentCallback(bool result);
void onDownlinkRecievedCallback(byte* recievedData, unsigned int dataLen, byte fPort, int rssi, byte snr);
byte sendUplink(byte* sendBuffer, size_t bufferLen, bool confirmed);
byte assembleErrorPacket(byte error, byte* dataPtr);
byte assembleInitPacket(byte* dataPtr) ;



#endif