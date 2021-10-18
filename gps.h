/**
 * This file is part of Wio LoRaWan Field Tester.
 *
 *   Wio LoRaWan Field Tester is free software created by Paul Pinault aka disk91. 
 *   You can redistribute it and/or modify it under the terms of the 
 *   GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   any later version.
 *
 *  Wio LoRaWan Field Tester is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Wio LoRaWan Field Tester.  If not, see <https://www.gnu.org/licenses/>.
 *
 *  Author : Paul Pinault (disk91.com)
 */   

#include <Arduino.h>

#ifndef __GPS_H__
#define __GPS_H__

#define GPS_MAX_HDOP  200
#define GPS_MIN_SAT   3

void gpsSetup();
void gpsLoop();
uint64_t gpsEncodePosition48b();
bool gpsQualityIsGoodEnough();
#ifdef DEBUGGPS
char * gpsLastNMEA();
#endif

typedef struct {
  bool      isReady;        // when false, no need to process these informations
  bool      rxStuff;
  
  uint32_t  updateTime;     // last update time in second within the day
  uint8_t   hour;
  uint8_t   minute;
  uint8_t   second;

  uint16_t  hdop;           // Hdop x 100
  int32_t   longitude;      // 1 / 10_000_000
  int32_t   latitude;       // 1 / 10_000_000
  int16_t   altitude;       // in meters
  uint8_t   sats;           // Number of sats used
  char *    lastNMEA;       // Last NMEA String received
  
} gpsData_t;

extern gpsData_t gps;
#endif
