#include "LoRaWAN_Handler.h"
#include <Arduino.h>

bool doOTAA = true;
DeviceClass_t g_CurrentClass = CLASS_C;                 /* class definition*/
LoRaMacRegion_t g_CurrentRegion = LORAMAC_REGION_US915; /* Region:US915*/
lmh_confirm g_CurrentConfirm = LMH_UNCONFIRMED_MSG;     /* confirm/unconfirm packet definition*/
uint8_t gAppPort = 1;                                   /* data port*/

/**@brief Structure containing LoRaWan parameters, needed for lmh_init()
*/
static lmh_param_t g_lora_param_init = { ADR_MODE, LORAWAN_DATARATE, LORAWAN_PUBLIC_NETWORK, JOINREQ_NBTRIALS, LORAWAN_TX_POWER, LORAWAN_DUTYCYCLE_OFF };


/**@brief Structure containing LoRaWan callback functions, needed for lmh_init()
*/
static lmh_callback_t g_lora_callbacks = { lorawan_get_battery_level, BoardGetUniqueId, BoardGetRandomSeed,
                                           lorawan_rx_handler, lorawan_has_joined_handler,
                                           lorawan_confirm_class_handler, lorawan_join_failed_handler,
                                           lorawan_unconf_finished, lorawan_conf_finished };
//OTAA keys !!!! KEYS ARE MSB !!!!
// AC1F09FFFE05159C
// uint8_t nodeDeviceEUI[8] = { 0xAC, 0x1F, 0x09, 0xFF, 0xFE, 0x05, 0x15, 0x9B };
// uint8_t nodeAppEUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
// uint8_t nodeAppKey[16] = { 0x55, 0x72, 0x40, 0x4C, 0x69, 0x6E, 0x6B, 0x4C, 0x6F, 0x52, 0x61, 0x32, 0x30, 0x31, 0x38, 0x23 };



static uint8_t m_lora_app_data_buffer[LORAWAN_APP_DATA_BUFF_SIZE];               //< Lora user application data buffer.
static lmh_app_data_t m_lora_app_data = { m_lora_app_data_buffer, 0, 0, 0, 0 };  //< Lora user application data structure.

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

  // Set LoRaWAN channel
  if (!lmh_setSubBandChannels(CHANNEL_MASK)) {
    Serial.println("Failed to set sub band.");
    return false;
  } else {
    Serial.printf("Set LoRaWAN Channel mask to %d\r\n", CHANNEL_MASK);
  }

  // Start Join procedure
  lmh_join();
  Serial.print("Setup finished for ");
  for (unsigned int i = 0; i < sizeof(nodeDeviceEUI); i++) {
    Serial.printf("0x%02hhx ", nodeDeviceEUI[i]);
  }
  return true;
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
    Serial.println("OTAA Mode, Network Joined!");
    digitalWrite(PIN_LED2, 0);
    lmh_error_status ret = lmh_class_request(g_CurrentClass);
    if (ret == LMH_SUCCESS) {
      delay(30000);

      byte data[10];
      byte len = assembleInitPacket(data);
      send_lora_frame(data, len, true);

    } else {
    }
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
/**@brief Downlink Handler

   @param[in] app_data  Pointer to rx data
*/
void lorawan_rx_handler(lmh_app_data_t* app_data) {
  Serial.printf("Downlink received on port %d, size:%d, rssi:%d, snr:%d, data:%s\n",
                app_data->port, app_data->buffsize, app_data->rssi, app_data->snr, app_data->buffer);

  if (processDownlinkPacket(app_data->buffer, app_data->buffsize)) {
    Serial.println("Downlink processing succcessful");
    printSummary();
  } else {
    Serial.println("Unknown or invalid downlink command");
  }
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
  Serial.println("TX finished");
  linkCheckCount++;
}

// Confirmed finished CALLBACK
void lorawan_conf_finished(bool result) {
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

/* Send LoRaWAN payload */
LoRaWAN_Send_Status send_lora_frame(byte* sendBuffer, int bufferLen, bool confirmed) {
  Serial.println("Check join..");
  if (lmh_join_status_get() != LMH_SET) {
    Serial.println("Not joined...");
    return NOT_JOINED;
  }

  Serial.println("Check buffer..");
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
      Serial.printf("lmh_send fail, Error:%d, fail count %d\n", error, count_fail);
      result = SEND_FAILED;
      break;
    case LMH_BUSY:
      count_fail++;
      Serial.printf("lmh_send busy, Error:%d, fail count %d\n", error, count_fail);
      result = SEND_FAILED;
      break;
    default:
      count_fail++;
      Serial.printf("lmh_send unknown error, Error:%d, fail count %d\n", error, count_fail);
      result = SEND_FAILED;
      break;
  }
  return result;
}
