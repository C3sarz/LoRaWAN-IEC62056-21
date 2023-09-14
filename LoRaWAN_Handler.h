#include <Arduino.h>
#include "LoRaWan-Arduino.h" //http://librarymanager/All#SX126x
#include <SPI.h>
#include <stdio.h>
#include "mbed.h"
#include "rtos.h"

#ifndef _LORAWAN_HANDLER_
#define _LORAWAN_HANDLER_

enum LoRaWAN_Send_Status {
  SEND_OK,
  SEND_FAILED,
  INVALID_DATA_LENGTH,
  NOT_JOINED,
};

// Definitions
#define LORAWAN_APP_DATA_BUFF_SIZE 100                     /**< buffer size of the data to be transmitted. */
#define LORAWAN_APP_INTERVAL 20000                        /**< Defines for user timer, the application data transmission interval. 20s, value in [ms]. */

// Foward declaration
static void lorawan_has_joined_handler(void);
static void lorawan_join_failed_handler(void);
static void lorawan_rx_handler(lmh_app_data_t *app_data);
static void lorawan_confirm_class_handler(DeviceClass_t Class);
LoRaWAN_Send_Status send_lora_frame(byte* sendBuffer, int bufferLen);
void lorawan_unconf_finished(void);
void lorawan_conf_finished(bool result);
bool setupLoRaWAN();

#endif