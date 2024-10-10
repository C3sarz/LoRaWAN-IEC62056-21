#ifndef _CONFIG_
#define _CONFIG_
#include <Arduino.h>
#pragma GCC poison malloc

enum SUPPORTED_REGIONS {
  REGIONS_AU915 = 0x0A,
  REGIONS_US915,
};

// System config
#define BATTERY_INSTALLED 0
#define CONFIRMED_COUNT 30
#define LOGLEVEL 2
#define MAX_QUERY_ATTEMPTS 3

// Storage config
#define INITIAL_PERIOD_MINUTES 1
#define CODES_LIMIT 16
#define STRING_MAX_SIZE 16
#define DEFAULT_BAUD_INDEX 0

// LoRa & LoRaWAN config
#define FREQUENCY_REGION REGIONS_AU915
#define DEFAULT_DATARATE 3
#define ADR_MODE 0
#define LORAWAN_SUBCHANNEL 2           /* Channels 8-15, (Milesight default) */
#define LORAWAN_APP_DATA_BUFF_SIZE 100 /**< buffer size of the data to be transmitted. */
#define LORAWAN_DEVICE_CLASS 2
#define RANDOM_TIME_DEVIATION_MAX 10000

extern uint8_t nodeDeviceEUI[8];
extern uint8_t nodeAppEUI[8];
extern uint8_t nodeAppKey[16];
typedef char CodeString[STRING_MAX_SIZE];

#endif