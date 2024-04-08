
#ifndef _LORAWAN_IMPLEMENTATION_
#define _LORAWAN_IMPLEMENTATION_
#include <Arduino.h>
#include "config.h"
#include <stdio.h>


enum LoRaWAN_Send_Status {
  SEND_OK,
  SEND_FAILED_NOT_JOINED,
  SEND_FAILED_BUSY,
  SEND_FAILED_GENERIC,
  INVALID_DATA_LENGTH,
};

extern volatile uint linkCheckCount;


// Callbacks
typedef struct LoRaWAN_Callbacks_s {
  void (*onJoinConfirmedCallback)(void);
  void (*onJoinFailedCallback)(void);
  void (*onUnconfirmedSentCallback)(void);
  void (*onConfirmedSentCallback)(bool result);
  void (*onDownlinkRecievedCallback)(byte* recievedData, unsigned int dataLen, byte fPort, int rssi, byte snr);
} LoRaWAN_Callbacks;

// Function Prototypes
bool ISetupLoRaWAN(LoRaWAN_Callbacks* newCallbacksPtr);
void IJoinNetwork(void);
LoRaWAN_Send_Status ISendLoRaWAN(byte* sendBuffer, size_t bufferLen, bool confirmed);

#endif