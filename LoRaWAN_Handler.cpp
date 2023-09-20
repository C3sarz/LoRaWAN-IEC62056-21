#include "LoRaWAN_Handler.h"

using namespace std::chrono_literals;
using namespace std::chrono;

bool doOTAA = true;                                     /**< Number of trials for the join request. */
DeviceClass_t g_CurrentClass = CLASS_A;                 /* class definition*/
LoRaMacRegion_t g_CurrentRegion = LORAMAC_REGION_US915; /* Region:US915*/
lmh_confirm g_CurrentConfirm = LMH_UNCONFIRMED_MSG;       /* confirm/unconfirm packet definition*/
uint8_t gAppPort = 85;                                  /* data port*/

/**@brief Structure containing LoRaWan parameters, needed for lmh_init()
*/
static lmh_param_t g_lora_param_init = { LORAWAN_ADR_OFF, LORAWAN_DATARATE, LORAWAN_PUBLIC_NETWORK, JOINREQ_NBTRIALS, LORAWAN_TX_POWER, LORAWAN_DUTYCYCLE_OFF };


/**@brief Structure containing LoRaWan callback functions, needed for lmh_init()
*/
static lmh_callback_t g_lora_callbacks = { BoardGetBatteryLevel, BoardGetUniqueId, BoardGetRandomSeed,
                                           lorawan_rx_handler, lorawan_has_joined_handler,
                                           lorawan_confirm_class_handler, lorawan_join_failed_handler,
                                           lorawan_unconf_finished, lorawan_conf_finished };
//OTAA keys !!!! KEYS ARE MSB !!!!
uint8_t nodeDeviceEUI[8] = { 0xAC, 0x1F, 0x09, 0xFF, 0xFE, 0x06, 0xB7, 0x69 };
uint8_t nodeAppEUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t nodeAppKey[16] = { 0x55, 0x72, 0x40, 0x4C, 0x69, 0x6E, 0x6B, 0x4C, 0x6F, 0x52, 0x61, 0x32, 0x30, 0x31, 0x38, 0x23 };

// ABP keys
uint32_t nodeDevAddr = 0x260116F8;
uint8_t nodeNwsKey[16] = { 0x7E, 0xAC, 0xE2, 0x55, 0xB8, 0xA5, 0xE2, 0x69, 0x91, 0x51, 0x96, 0x06, 0x47, 0x56, 0x9D, 0x23 };
uint8_t nodeAppsKey[16] = { 0xFB, 0xAC, 0xB6, 0x47, 0xF3, 0x58, 0x45, 0xC7, 0x50, 0x7D, 0xBF, 0x16, 0x8B, 0xA8, 0xC1, 0x7C };

static uint8_t m_lora_app_data_buffer[LORAWAN_APP_DATA_BUFF_SIZE];               //< Lora user application data buffer.
static lmh_app_data_t m_lora_app_data = { m_lora_app_data_buffer, 0, 0, 0, 0 };  //< Lora user application data structure.

// mbed::Ticker appTimer;
// void tx_lora_periodic_handler(void);

static uint32_t count = 0;
static uint32_t count_fail = 0;


// LoRaWAN Hardware and Software setup
bool setupLoRaWAN() {

  // Simple RAK11300 Setup
  lora_rak11300_init();
  lmh_setDevEui(nodeDeviceEUI);
  lmh_setAppEui(nodeAppEUI);
  lmh_setAppKey(nodeAppKey);

  // Initialize LoRaWan
  uint32_t err_code = lmh_init(&g_lora_callbacks, g_lora_param_init, doOTAA, g_CurrentClass, g_CurrentRegion);
  if (err_code != 0) {
    Serial.printf("lmh_init failed - %d\n", err_code);
    return false;
  }

  if (!lmh_setSubBandChannels(CHANNEL_MASK)) {
    Serial.println("Failed to set sub band.");
    return false;
  }
  else{
    Serial.printf("Set LoRaWAN Channel mask to %d\r\n",CHANNEL_MASK);
  }

  // Start Join procedure
  lmh_join();
  Serial.println("Setup finished.");
  return true;
}

/**@brief LoRa function for handling HasJoined event.
*/
void lorawan_has_joined_handler(void) {

  Serial.println("OTAA Mode, Network Joined!");


  lmh_error_status ret = lmh_class_request(g_CurrentClass);
  if (ret == LMH_SUCCESS) {
    delay(1000);
    // Start the application timer. Time has to be in microseconds
    // appTimer.attach(tx_lora_periodic_handler, (std::chrono::microseconds)(LORAWAN_APP_INTERVAL * 1000));
  } else {
    Serial.println("Error while setting up periodic timer.");
  }
}
/**@brief LoRa function for handling OTAA join failed
*/
void lorawan_join_failed_handler(void) {
  Serial.println("OTAA join failed!");
  Serial.println("Check your EUI's and Keys's!");
  Serial.println("Check if a Gateway is in range!");
  lmh_join();
}
/**@brief Function for handling LoRaWan received data from Gateway

   @param[in] app_data  Pointer to rx data
*/
void lorawan_rx_handler(lmh_app_data_t* app_data) {
  Serial.printf("Downlink received on port %d, size:%d, rssi:%d, snr:%d, data:%s\n",
                app_data->port, app_data->buffsize, app_data->rssi, app_data->snr, app_data->buffer);
}

void lorawan_confirm_class_handler(DeviceClass_t Class) {
  Serial.printf("switch to class %c done\n", "ABC"[Class]);
  // Informs the server that switch has occurred ASAP
  m_lora_app_data.buffsize = 0;
  m_lora_app_data.port = gAppPort;
  lmh_send(&m_lora_app_data, g_CurrentConfirm);
}

void lorawan_unconf_finished(void) {
  Serial.println("TX finished");
}

void lorawan_conf_finished(bool result) {
  Serial.printf("Confirmed TX %s\n", result ? "success" : "failed");
}

// =====================================================================
/* Send LoRaWAN payload */
LoRaWAN_Send_Status send_lora_frame(byte* sendBuffer, int bufferLen) {
  Serial.println("Check join..");
  if (lmh_join_status_get() != LMH_SET) {
    Serial.println("Not joined...");
    //Not joined, try again later
    return NOT_JOINED;
  }

  Serial.println("Check buffer..");
  if (bufferLen > LORAWAN_APP_DATA_BUFF_SIZE) {
    return INVALID_DATA_LENGTH;
  }
  m_lora_app_data.port = gAppPort;
  memcpy(m_lora_app_data.buffer, sendBuffer, bufferLen);
  m_lora_app_data.buffsize = bufferLen;
  Serial.printf("Data copied to LoRa buffer, s:%u\r\n", bufferLen);

  lmh_error_status error = lmh_send(&m_lora_app_data, g_CurrentConfirm);
  Serial.println("sent..");
  if (error == LMH_SUCCESS) {
    count++;
    Serial.printf("lmh_send enqueued count %d\n", count);
    return SEND_OK;
  } else {
    count_fail++;
    Serial.printf("lmh_send fail, R:%d, count %d\n", error, count_fail);
    return SEND_FAILED;
  }
}

/**@brief Function for handling user timerout event.
*/
// void tx_lora_periodic_handler(void) {
//   appTimer.attach(tx_lora_periodic_handler, (std::chrono::microseconds)(LORAWAN_APP_INTERVAL * 1000));
//   send_now = true;
// }