#include "LoRaWAN_Implementation.h"
#include <stdio.h>
#include "LoRaWan-Arduino.h"  //http://librarymanager/All#SX126x
#include "config.h"

#define LORAWAN_TX_POWER TX_POWER_0 /*LoRaMac tx power definition, from TX_POWER_0 to TX_POWER_15*/
#define JOINREQ_NBTRIALS 3

DeviceClass_t g_CurrentClass = CLASS_C;                 /* class definition*/
// LoRaMacRegion_t g_CurrentRegion = LORAMAC_REGION_US915; /* Region:US915*/
lmh_confirm g_CurrentConfirm = LMH_UNCONFIRMED_MSG;     /* confirm/unconfirm packet definition*/
uint8_t gAppPort = 1;                                   /* data port*/
extern const LoRaMacParams_t LoRaMacParams;

uint8_t lorawan_get_battery_level(void);
void lorawan_has_joined_handler(void);
void lorawan_join_failed_handler(void);
void lorawan_rx_handler(lmh_app_data_t* app_data);
void lorawan_confirm_class_handler(DeviceClass_t Class);
void lorawan_unconf_finished(void);
void lorawan_conf_finished(bool result);

/**@brief Structure containing LoRaWan parameters, needed for lmh_init()
*/
static lmh_param_t g_lora_param_init = { ADR_MODE, DEFAULT_DATARATE, LORAWAN_PUBLIC_NETWORK, JOINREQ_NBTRIALS, LORAWAN_TX_POWER, LORAWAN_DUTYCYCLE_OFF };


/**@brief Structure containing LoRaWan callback functions, needed for lmh_init()
*/
static lmh_callback_t g_lora_callbacks = {
  BATTERY_INSTALLED ? lorawan_get_battery_level : NULL,
  BoardGetUniqueId,
  BoardGetRandomSeed,
  lorawan_rx_handler,
  lorawan_has_joined_handler,
  lorawan_confirm_class_handler,
  lorawan_join_failed_handler,
  lorawan_unconf_finished,
  lorawan_conf_finished,
};

LoRaWAN_Callbacks* callbacksPtr;

static uint8_t m_lora_app_data_buffer[LORAWAN_APP_DATA_BUFF_SIZE];               //< Lora user application data buffer.
static lmh_app_data_t m_lora_app_data = { m_lora_app_data_buffer, 0, 0, 0, 0 };  //< Lora user application data structure.

static uint32_t count = 0;
static uint32_t count_fail = 0;
byte currentDR = DEFAULT_DATARATE;

// LoRaWAN Hardware and Software setup
bool ISetupLoRaWAN(LoRaWAN_Callbacks* newCallbacksPtr) {

// Region selection from config
LoRaMacRegion_t g_CurrentRegion = LORAMAC_REGION_AU915; /* Default Region:AU915*/
  switch(FREQUENCY_REGION){
    case REGIONS_AU915:
      g_CurrentRegion = LORAMAC_REGION_AU915;
      break;
      case REGIONS_US915:
      g_CurrentRegion = LORAMAC_REGION_US915;
      break;
      default:
      g_CurrentRegion = LORAMAC_REGION_AU915;
      break;
  }

  // Simple RAK11300 Setup
  lora_rak11300_init();
  lmh_setDevEui(nodeDeviceEUI);
  lmh_setAppEui(nodeAppEUI);
  lmh_setAppKey(nodeAppKey);

  // Initialize LoRaWan
  uint32_t err_code = lmh_init(&g_lora_callbacks, g_lora_param_init, true, g_CurrentClass, g_CurrentRegion);
  if (err_code != 0) {
    Serial.printf("lmh_init failed - %d\n", err_code);
    return false;
  }

  // Set LoRaWAN channel
  if (!lmh_setSubBandChannels(LORAWAN_SUBCHANNEL)) {
    Serial.println("Failed to set sub band.");
    return false;
  } else {
    Serial.printf("Set LoRaWAN Channel mask to %d\r\n", LORAWAN_SUBCHANNEL);
  }

  callbacksPtr = newCallbacksPtr;
  return true;
}

void IJoinNetwork(void) {
  lmh_join();
}

/**@brief LoRa function for handling HasJoined event.
*/
uint8_t lorawan_get_battery_level(void) {
  // uint16_t vbat = getVBatInt();
  return 254;
}

/**@brief LoRa function for handling HasJoined event.
*/
void lorawan_has_joined_handler(void) {
  static bool joined = false;
  if (!joined) {
    joined = true;
    callbacksPtr->onJoinConfirmedCallback();
  }
}
/**@brief LoRa function for handling OTAA join failed
*/
void lorawan_join_failed_handler(void) {
  callbacksPtr->onJoinFailedCallback();
}
/**@brief Downlink Handler

   @param[in] app_data  Pointer to rx data
*/
void lorawan_rx_handler(lmh_app_data_t* app_data) {
  Serial.printf("Downlink received on port %d, size:%d, rssi:%d, snr:%d, data:%s\n",
                app_data->port, app_data->buffsize, app_data->rssi, app_data->snr, app_data->buffer);

  callbacksPtr->onDownlinkRecievedCallback(app_data->buffer, app_data->buffsize, app_data->port, app_data->rssi, app_data->snr);
}

// LoRaWAN device type switch handler
void lorawan_confirm_class_handler(DeviceClass_t Class) {
  Serial.printf("switch to class %c done\n", "ABC"[Class]);
  m_lora_app_data.buffsize = 0;
  m_lora_app_data.port = gAppPort;
  lmh_send(&m_lora_app_data, g_CurrentConfirm);
}

// Unconfirmed finished CALLBACK
void lorawan_unconf_finished(void) {
  callbacksPtr->onUnconfirmedSentCallback();
}

// Confirmed finished CALLBACK
void lorawan_conf_finished(bool result) {
  callbacksPtr->onConfirmedSentCallback(result);
}

/* Send LoRaWAN payload */
LoRaWAN_Send_Status ISendLoRaWAN(byte* sendBuffer, size_t bufferLen, bool confirmed) {
  if (lmh_join_status_get() != LMH_SET) {
    return SEND_FAILED_NOT_JOINED;
  }

  if (bufferLen > LORAWAN_APP_DATA_BUFF_SIZE) {
    return INVALID_DATA_LENGTH;
  }

  // Set parameters
  m_lora_app_data.port = gAppPort;
  memcpy(m_lora_app_data.buffer, sendBuffer, bufferLen);
  m_lora_app_data.buffsize = bufferLen;
  Serial.printf("Data copied to LoRa buffer, s:%u\r\n", bufferLen);

  // Set confirmed/unconfirmed
  lmh_confirm confirmType = confirmed ? LMH_CONFIRMED_MSG : LMH_UNCONFIRMED_MSG;

  // Send packet
  lmh_error_status error = lmh_send(&m_lora_app_data, confirmType);
  LoRaWAN_Send_Status result;
  switch (error) {
    case LMH_SUCCESS:
      count++;
      Serial.printf("lmh_send enqueued count %d\n", count);
      result = SEND_OK;
      break;
    case LMH_ERROR:
      count_fail++;
      // Serial.printf("lmh_send fail, Error:%d, fail count %d\n", error, count_fail);
      result = SEND_FAILED_GENERIC;
      break;
    case LMH_BUSY:
      count_fail++;
      // Serial.printf("lmh_send busy, Error:%d, fail count %d\n", error, count_fail);
      result = SEND_FAILED_BUSY;
      break;
    default:
      count_fail++;
      // Serial.printf("lmh_send unknown error, Error:%d, fail count %d\n", error, count_fail);
      result = SEND_FAILED_GENERIC;
      break;
  }
  return result;
}

byte getCurrentDatarate(void){
  byte dr = static_cast<byte>(LoRaMacParams.ChannelsDatarate);
  return dr;
}
