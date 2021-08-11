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


//#define DEBUG
//#define DEBUGGPS

#define RFM95_NSS_PIN   0
#define RFM95_RST_PIN   1
#define RFM95_DIO_0     3
#define RFM95_DIO_1     2
#define LIPO_ADC        A4

#define WITH_SPLASH         1
#define WITH_SPLASH_HELIUM  1   
//#define WITH_SPLASH_TTN     1

#define WITH_GPS
#define WITH_LIPO

#ifdef DEBUG
  #define LOGLN(x)  Serial.println x
  #define LOG(x) Serial.print x
#else
  #define LOGLN(x) 
  #define LOG(x)
#endif

#define SERIALCONFIG  Serial

#define US915_DUTYCYCLE_MS 10000    // Fair use and preserve DCs in MS

bool readConfig();
void storeConfig();

#endif // __CONFIG_H
