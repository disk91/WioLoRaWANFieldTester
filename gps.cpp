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

#include "config.h"
#include "gps.h"

gpsData_t gps;

#ifndef WITH_GPS

void gpsSetup() {
  gps.hasbeenReady = false;
  gps.isReady = false;
}

void gpsLoop() {
  // just do nothing
}

uint64_t gpsEncodePosition48b() {
  return 0;
}

#else
#include <Adafruit_GPS.h>

#if HWTARGET == LORAE5
  #ifndef ESP8266
  #error "you need to add #define ESP8266 in the Adafruit_GPS.h file in AdafruitGPS library"
  #endif
  #include <SoftwareSerial.h>
  SoftwareSerial softSerial(3,2);
  #define GPSSerial softSerial
#else
  #define GPSSerial Serial1
#endif

Adafruit_GPS GPS(&GPSSerial);

// empty pending char from GPS with a timeout before leaving
void clearGpsPendingChar(uint32_t timeout) {
  uint32_t start = millis();
  while ( GPS.available() || (millis() - start) < timeout ) {
    if ( GPS.available() ) {
      start = millis();
      GPS.read();
    }
  }
}

#ifdef DEBUGGPS
char * gpsLastNMEA() { 
  return GPS.lastNMEA();
};
#endif

void gpsSetup() {
  #if HWTARGET == LORAE5
    #if _SS_MAX_RX_BUFF < 128
    #error "You must change the SoftSerial Buffer size in SoftSerial.h for 128"
    #endif
    delay(250);
    GPS.begin(9600);
    GPSSerial.listen();
    delay(1200);
  #else
    GPS.begin(9600);
    delay(2500);
  #endif

  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_GGAONLY);
  clearGpsPendingChar(100);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_GGAONLY);  // make sure
  clearGpsPendingChar(100);
  //GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_200_MILLIHERTZ); // 5s update rate
  clearGpsPendingChar(100);
  // Request no updates on antenna status, comment out to keep quiet
  GPS.sendCommand(PGCMD_NOANTENNA);
  clearGpsPendingChar(100);
  // Request no update on GPTXT
  GPS.sendCommand("$PQTXT,W,0,0*22");
  clearGpsPendingChar(100);
  
  delay(500);
  gps.hasbeenReady = false;
  gps.isReady = false;
  gps.rxStuff = false;
}
  

void gpsLoop() {
  
  char c = GPS.read();
  
  //Serial.print(c);
  if (GPS.newNMEAreceived()) {
    // Since this function change internal library var,
    // avoid multiple call, just once and use results later
    gps.lastNMEA = GPS.lastNMEA();
    LOGGPS((gps.lastNMEA));
    gps.rxStuff = true;
    if (!GPS.parse(gps.lastNMEA)) // this also sets the newNMEAreceived() flag to false
      return; // we can fail to parse a sentence in which case we should just wait for another
  }

  // update the position information
  if ( GPS.fix == 1 ) {
    // GPS is ready
    uint32_t cTime = GPS.hour * 3600 + GPS.minute * 60 + GPS.seconds; 
    if ( ! gps.isReady || cTime != gps.updateTime ) {
      gps.hdop = (uint16_t)(GPS.HDOP*100.0);
      if ( GPS.satellites >= 2 ) {
          gps.isReady = true;
          gps.hasbeenReady = true;
          gps.updateTime = cTime;
          gps.hour = GPS.hour;
          gps.minute = GPS.minute;
          gps.second = GPS.seconds;
          gps.altitude = (int16_t) GPS.altitude;
          gps.longitude = GPS.longitude_fixed;
          gps.latitude = GPS.latitude_fixed;
          gps.sats = GPS.satellites;
          #ifdef DEBUGGPS
          Serial.printf("La: %d Lo: %d alt: %d sat: %d hdop: %d\n",
             gps.latitude,
             gps.longitude,
             gps.altitude,
             gps.sats,
             gps.hdop
          );
          #endif
      } else {
        gps.isReady = false;
      }    
    }
  } else {
    gps.isReady = false;
  }
  
}

void gpsBackupPosition() {
  gps.backupLongitude = gps.longitude;
  gps.backupLatitude = gps.latitude;
}


// really low quality distance estimator
// return a result in meter
int gpsEstimateDistance() {

  int32_t dLon = gps.backupLongitude - gps.longitude;
  int32_t dLat = gps.backupLatitude - gps.latitude;
  if (dLon < 0) dLon = -dLon;
  if (dLat < 0) dLat = -dLat;
  dLon += dLat; 
  // we can estimate that a value of 100 = 1m
  if ( dLon > 1000000 ) dLon = 1000000; // no need to be over 10km / preserve UI display in debug
  return (dLon/100); 
  
}


bool gpsQualityIsGoodEnough() {

  return (gps.isReady && gps.hdop < GPS_MAX_HDOP && gps.sats > GPS_MIN_SAT ); 
  
}
/**
 * Compact encoding of the current position
 * The result is stored in the **output** uint64_t variable
 * the result is stored in 0x0000_FFFF_FFFF_FFFF
 * See https://www.disk91.com/2015/technology/sigfox/telecom-design-sdk-decode-gps-frame/
 *  for encoding detail
 * Basically
 * 444444443333333333222222222211111111110000000000
 * 765432109876543210987654321098765432109876543210
 * X                                                - lng Sign 1=-
 *  X                                               - lat Sign 1=-
 *   XXXXXXXXXXXXXXXXXXXXXXX                        - 23b Latitude
 *                          XXXXXXXXXXXXXXXXXXXXXXX - 23b Longitude
 *
 *  division by 215 for longitude is to get 180*10M to fit in 2^23b
 *  subtraction of 107 is 0.5 * 215 to round the value and not always be floored.
 */
uint64_t gpsEncodePosition48b() {

  uint64_t t = 0;
  uint64_t l = 0;
  if ( gps.longitude < 0 ) {
    t |= 0x800000000000L;
    l = -gps.longitude;
  } else {
    l = gps.longitude;
  }
  if ( l/10000000 >= 180  ) {
    l = 8372093;
  } else {
    if ( l < 107 ) {
      l = 0;
    } else {
      l = (l - 107) / 215;
    }
  }
  t |= (l & 0x7FFFFF );

  if ( gps.latitude < 0 ) {
    t |= 0x400000000000L;
    l = -gps.latitude;
  } else {
    l = gps.latitude;
  }
  if ( l/10000000 >= 90  ) {
    l = 8333333;
  } else {
    if ( l < 53 ) {
      l = 0;
    } else {
      l = (l - 53) / 108;
    }
  }
  t |= (l << 23) & 0x3FFFFF800000;

  return t;
}

#endif
