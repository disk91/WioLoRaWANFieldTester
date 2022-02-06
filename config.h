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
#ifndef __CONFIG_H
#define __CONFIG_H

// 1 - select if you want some debug
//#define DEBUG
//#define DEBUGGPS
//#define DEBUGLORA
//#define DEBUGDATA

#define RFM95      0      // Make sure the board ROLE is MASTER
#define LORAE5     1      // Make sure the board ROLE is SLAVE

// 2 - select the target board, original one based on RFM95 or Seed LoRa E5 version
#define HWTARGET   LORAE5

#if HWTARGET == LORAE5
  #define CFG_anyZone
  #define WITH_GPS
  #define SERIALE5  Serial1
  #define WITH_LIPO
#endif

#if HWTARGET == RFM95
  #define RFM95_NSS_PIN   0
  #define RFM95_RST_PIN   1
  #define RFM95_DIO_0     3
  #define RFM95_DIO_1     2
  #define LIPO_ADC        A4

  // Offset to add when calculating battery percent remaining (in mv) 
  // Sometimes R/R divider may not accurate and battery voltage can be
  // shifted, so percent calculation may be wrong, this offset is added
  // on each calculation to fix it, best way to know this one if to fully
  // charge battery should be 4.2V, and read on GPS screen battery voltage
  // to view difference, in my case reading was 3950mV so added 250mV offset
  // it's empiric of course, not best but better than nothing
  // #define LIPO_OFFSET_MV 250 

// 3 - select possible options
  #define WITH_GPS
  #define WITH_LIPO
#endif

// 4 - Active on of the possible Splash screen ( or none or both )
#define WITH_SPLASH         1
#define WITH_SPLASH_HELIUM  1   
//#define WITH_SPLASH_TTN     1
//#define JUSTCLEAN


#define VERSION "v1.5"

#ifdef DEBUG
  #define LOGLN(x)  Serial.println x
  #define LOG(x) Serial.print x
  #define LOGF(x) Serial.printf x
#else
  #define LOGLN(x) 
  #define LOG(x)
  #define LOGF(x)
#endif

#ifdef DEBUGLORA
  #define LOGLORALN(x)  Serial.println x
  #define LOGLORA(x) Serial.print x
  #define LOGLORAF(x) Serial.printf x
#else
  #define LOGLORALN(x) 
  #define LOGLORA(x)
  #define LOGLORAF(x)
#endif

#ifdef DEBUGGPS
  #define LOGGPSLN(x) Serial.println x
  #define LOGGPS(x) Serial.print x
  #define LOGGPSF(x) Serial.printf x
#else
  #define LOGGPSLN(x) 
  #define LOGGPS(x)
  #define LOGGPSF(x)
#endif

#define SERIALCONFIG  Serial

#define NONDCZONE_DUTYCYCLE_MS 25000                     // Fair use and preserve DCs in MS
#define MAXNONMOVEMENT_DURATION_MS ( 30 * 60 * 1000 )    // Fair use - downgrade period when the position of the device is unchanged

bool readConfig();
void storeConfig();
bool storeConfigToBackup();
bool readConfigFromBackup();
void clearBackup();

#if HWTARGET == LORAE5
  void processLoRaE5GpsFix();
#endif

#endif // __CONFIG_H
