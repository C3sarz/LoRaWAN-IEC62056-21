
#ifndef _STORAGE_IMPLEMENTATION_
#define _STORAGE_IMPLEMENTATION_

#include <Arduino.h>
#include "config.h"

class StoredConfig {
public:
  char codes[CODES_LIMIT][STRING_MAX_SIZE];
  byte uplinkMinutes;
  byte baudIndex;

  StoredConfig(void) {
  }
};

bool IReadStoredValues(StoredConfig* config);
bool IWriteStoredValues(StoredConfig config);
bool checkIfDataChanged(StoredConfig config);

#endif