#include "config.h"

const char* fw_version = "1.0";

//OTAA keys !!!! KEYS ARE MSB !!!!
// AC1F09FFFE0512A0 sub ayolas
uint8_t nodeDeviceEUI[8] = { 0xAC, 0x1F, 0x09, 0xFF, 0xFE, 0x05, 0x15, 0x9E };
uint8_t nodeAppEUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t nodeAppKey[16] = { 0x55, 0x72, 0x40, 0x4C, 0x69, 0x6E, 0x6B, 0x4C, 0x6F, 0x52, 0x61, 0x32, 0x30, 0x31, 0x38, 0x23 };

const char* defaultCodes[] = {
  "ignore", //1
  "ignore", //2
  "ignore", //3
  "ignore", //4
  "ignore", //5
  "ignore", //6
  "32.7.0",   //7
  "52.7.0",   //8
  "72.7.0",   //9
  "15.7.0",   //10
  "15.5.0",   //11
  "15.8.0",   //12
  "15.6.0",   //13
  "13.5.0",   //14
  "15.8.0*02",//15
  "ignore"    //16
};