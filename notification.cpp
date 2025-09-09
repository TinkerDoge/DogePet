// Notification Face Module - Implementation
#include "notification.h"
#include "config.h"
#include <stdint.h>

// Ensure stdint types are available
#if !defined(__STDINT_H) && !defined(_STDINT_H_)
typedef unsigned long uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef long int32_t;
typedef short int16_t;
typedef signed char int8_t;
#endif

// returns whether we have any cached notification text
bool hasNotif() {
  return lastNotifTitle[0] != '\0' || lastNotifBody[0] != '\0';
}
// OLED drawing functions removed for TFT/LVGL migration
