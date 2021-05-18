#include "config.h"
#include "gps.h"

gpsData_t gps;

#ifndef WITH_GPS

void gpsSetup() {
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
#define GPSSerial Serial1
Adafruit_GPS GPS(&GPSSerial);


void gpsSetup() {
  GPS.begin(9600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_GGAONLY);
  //GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_200_MILLIHERTZ); // 5s update rate
  // Request no updates on antenna status, comment out to keep quiet
  GPS.sendCommand(PGCMD_NOANTENNA);
  // Request no update on GPTXT
  GPS.sendCommand("$PQTXT,W,0,0*22");
  
  delay(1000);
  gps.isReady = false;
}
  

void gpsLoop() {

  char c = GPS.read();
  if (GPS.newNMEAreceived()) {
    if (!GPS.parse(GPS.lastNMEA())) // this also sets the newNMEAreceived() flag to false
      return; // we can fail to parse a sentence in which case we should just wait for another
  }

  // update the position information
  if ( GPS.fix == 1 ) {
    // GPS is ready
    uint32_t cTime = GPS.hour * 3600 + GPS.minute * 60 + GPS.seconds; 
    if ( ! gps.isReady || cTime != gps.updateTime ) {
      gps.hdop = (uint16_t)(GPS.HDOP*100.0);
      if ( gps.hdop < 300 && GPS.satellites >= 4 ) {
          gps.isReady = true;
          gps.updateTime = cTime;
          gps.hour = GPS.hour;
          gps.minute = GPS.minute;
          gps.second = GPS.seconds;
          gps.altitude = (int16_t) GPS.altitude;
          gps.longitude = GPS.longitude_fixed;
          gps.latitude = GPS.latitude_fixed;
          gps.sats = GPS.satellites;
          #ifdef DEBUG
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
 *  substraction of 107 is 0.5 * 215 to round the value and not always be floored.
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
