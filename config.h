#ifndef _CONFIG_
#define _CONFIG_
#include <Arduino.h>

// System config
#define BATTERY_INSTALLED 1
#define CONFIRMED_COUNT 30
#define LOGLEVEL 1

// Storage config
#define INITIAL_PERIOD_MINUTES 1
#define CODES_LIMIT 16
#define STRING_MAX_SIZE 16
#define DEFAULT_BAUD_INDEX 0

// LoRa & LoRaWAN config
#define ADR_MODE LORAWAN_ADR_OFF
#define CHANNEL_MASK 2                 /* Channels 8-15, (Milesight default) */
#define LORAWAN_APP_DATA_BUFF_SIZE 100 /**< buffer size of the data to be transmitted. */
#define LORAWAN_DATARATE DR_3          /*LoRaMac datarates definition, from DR_0 to DR_5*/

extern uint8_t nodeDeviceEUI[8];
extern uint8_t nodeAppEUI[8];
extern uint8_t nodeAppKey[16];
typedef char CodeString[STRING_MAX_SIZE];

#endif