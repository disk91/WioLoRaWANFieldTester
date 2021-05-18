#include <Arduino.h>

#ifndef __GPS_H__
#define __GPS_H__

void gpsSetup();
void gpsLoop();
uint64_t gpsEncodePosition48b();

typedef struct {
  bool      isReady;        // when false, no need to process these informations
  
  uint32_t  updateTime;     // last update time in second within the day
  uint8_t   hour;
  uint8_t   minute;
  uint8_t   second;

  uint16_t  hdop;           // Hdop x 100
  int32_t   longitude;      // 1 / 10_000_000
  int32_t   latitude;       // 1 / 10_000_000
  int16_t   altitude;       // in meters
  uint8_t   sats;           // Number of sats used
  
} gpsData_t;

extern gpsData_t gps;
#endif
