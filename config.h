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
// 3 - select possible options
  #define WITH_GPS
  #define WITH_LIPO
#endif

// 4 - Active on of the possible Splash screen ( or none or both )
#define WITH_SPLASH         1
#define WITH_SPLASH_HELIUM  1   
//#define WITH_SPLASH_TTN     1
//#define JUSTCLEAN


#define VERSION "v1.4a"

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


#define SERIALCONFIG  Serial

#define NONDCZONE_DUTYCYCLE_MS 25000    // Fair use and preserve DCs in MS

bool readConfig();
void storeConfig();
bool storeConfigToBackup();
bool readConfigFromBackup();
void clearBackup();

#endif // __CONFIG_H
