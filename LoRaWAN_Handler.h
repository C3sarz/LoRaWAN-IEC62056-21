
#ifndef _LORAWAN_HANDLER_
#define _LORAWAN_HANDLER_
#include <Arduino.h>
#include "Storage.h"
#include "LoRaWan-Arduino.h"  //http://librarymanager/All#SX126x
#include <SPI.h>
#include <stdio.h>
#include "mbed.h"
#include "rtos.h"
#include "MeterInterface.h"
#include "Peripherals.h"

#define SCHED_MAX_EVENT_DATA_SIZE APP_TIMER_SCHED_EVENT_DATA_SIZE /**< Maximum size of scheduler events. */
#define SCHED_QUEUE_SIZE 60 /**< Maximum number of events in the scheduler queue. */
#define LORAWAN_TX_POWER TX_POWER_0 /*LoRaMac tx power definition, from TX_POWER_0 to TX_POWER_15*/
#define JOINREQ_NBTRIALS 3
#define RANDOM_TIME_MAX 10000

enum LoRaWAN_Send_Status {
  SEND_OK,
  SEND_FAILED,
  INVALID_DATA_LENGTH,
  NOT_JOINED,
};

// LoRaWAN Keys
extern uint8_t nodeDeviceEUI[8];
extern uint8_t nodeAppEUI[8];
extern uint8_t nodeAppKey[16];

// Function Prototypes
void lorawan_has_joined_handler(void);
void lorawan_join_failed_handler(void);
void lorawan_rx_handler(lmh_app_data_t* app_data);
void lorawan_confirm_class_handler(DeviceClass_t Class);
LoRaWAN_Send_Status send_lora_frame(byte* sendBuffer, int bufferLen, bool confirmed);
void lorawan_unconf_finished(void);
void lorawan_conf_finished(bool result);
bool setupLoRaWAN();

#endif