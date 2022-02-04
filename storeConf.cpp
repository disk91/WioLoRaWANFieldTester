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
#include <FlashStorage.h>
#include <lmic.h>
#include "config.h"
#include "testeur.h"
#include "ui.h"
#include "LoRaCom.h"

#define MAGIC 0xD154
#define VERSION 0x03

#define FLAG_BACKUPED 0x01
#define FLAG_HIDE_KEY 0x02

typedef struct {
  uint16_t  magic;
  uint8_t   version;
  uint8_t   csum;     // checksum after byte 4 ...
  
  int8_t    cPwr;     // Current Power
  uint8_t   cSf;      // Current SF
  uint8_t   cRetry;   // Current Number of retry
  uint8_t   selected_display;
  uint8_t   selected_mode;
  uint8_t   deveui[8];   // device EUI
  uint8_t   appeui[8];   // App EUI
  uint8_t   appkey[16];  // App KEY
  uint8_t   zone;        // Zone EU868... cf loracom.h ZONE_XXX
  uint8_t   flag;        // to store some flags
} Config;


FlashStorage(my_flash_store, Config);

uint8_t computeCSum(Config * c) {
    uint8_t * t = (uint8_t *)c;
    uint8_t csum = 0;
    for ( int k = 4 ; k < sizeof(Config); k++) {
      csum += t[k];
    }
    return csum;
}


// Load configuration from flash
// return true if loaded false if default must be set
bool readConfig() {

  Config c = my_flash_store.read();
  uint8_t csum = computeCSum(&c);
  if ( c.magic == MAGIC && c.version == VERSION && c.csum == csum ) {
    state.cPwr = c.cPwr;
    state.cSf = c.cSf;
    state.cRetry = c.cRetry;
    state.cnfBack = ((c.flag & FLAG_BACKUPED ) > 0);
    state.hidKey = ((c.flag & FLAG_HIDE_KEY ) > 0);
    ui.selected_display = c.selected_display;
    ui.selected_mode = c.selected_mode;
    memcpy(loraConf.deveui, c.deveui, 8);
    memcpy(loraConf.appeui, c.appeui, 8);
    memcpy(loraConf.appkey, c.appkey,16);
    loraConf.zone = c.zone;
  } else {
    return false;
  }
  return true;
}

void storeConfig() {
  Config c;
  c.magic = MAGIC;
  c.version = VERSION;
  c.cPwr = state.cPwr;
  c.cSf = state.cSf;
  c.cRetry = state.cRetry;
  c.selected_display = ui.selected_display;
  c.selected_mode = ui.selected_mode;
  c.zone = loraConf.zone;
  c.flag = 0;
  if ( state.cnfBack ) c.flag |= FLAG_BACKUPED;
  if ( state.hidKey ) c.flag |= FLAG_HIDE_KEY;

  memcpy(c.deveui, loraConf.deveui, 8);
  memcpy(c.appeui, loraConf.appeui, 8);
  memcpy(c.appkey, loraConf.appkey,16);
  c.csum = computeCSum(&c);
  
  my_flash_store.write(c);
}

#if HWTARGET == LORAE5

  // Use the LoRae5 internal storage to save the config and support firmware update
  bool readConfigFromBackup() {
  
    Config c;
    uint8_t * t = (uint8_t *) &c;

    if ( ! quickSetup() ) return false;

    for ( int k = 0 ; k < sizeof(Config); k++) {
       readOneByte(k, t);
       t++;
    }

    uint8_t csum = computeCSum(&c);
    if ( c.magic == MAGIC && c.version == VERSION && c.csum == csum ) {
      state.cPwr = c.cPwr;
      state.cSf = c.cSf;
      state.cRetry = c.cRetry;
      state.cnfBack = ((c.flag & FLAG_BACKUPED ) > 0);
      state.hidKey = ((c.flag & FLAG_HIDE_KEY ) > 0);
      ui.selected_display = c.selected_display;
      ui.selected_mode = c.selected_mode;
      memcpy(loraConf.deveui, c.deveui, 8);
      memcpy(loraConf.appeui, c.appeui, 8);
      memcpy(loraConf.appkey, c.appkey,16);
      loraConf.zone = c.zone;
    } else {
      return false;
    }
    return true;
      
  }
  
  bool storeConfigToBackup( ) {

    Config c;
    c.magic = MAGIC;
    c.version = VERSION;
    c.cPwr = state.cPwr;
    c.cSf = state.cSf;
    c.cRetry = state.cRetry;
    c.flag = FLAG_BACKUPED;
    if ( state.hidKey ) c.flag |= FLAG_HIDE_KEY;
    c.selected_display = ui.selected_display;
    c.selected_mode = ui.selected_mode;
    c.zone = loraConf.zone;
    memcpy(c.deveui, loraConf.deveui, 8);
    memcpy(c.appeui, loraConf.appeui, 8);
    memcpy(c.appkey, loraConf.appkey,16);
    c.csum = computeCSum(&c);

    uint8_t * t = (uint8_t *) &c;
    bool ret = true;
    for ( int k = 0 ; k < sizeof(Config); k++) {
       if ( ! storeOneByte(k,*t) ) {
         ret = false;
         break;
       }
       t++;
    }
    if ( ret ) {
      state.cnfBack = true;
    }
    return ret;
    
  }

  void clearBackup() {

     for ( int k = 0 ; k < sizeof(Config); k++) {
        storeOneByte(k,0);
     }
  
  }

#else

  bool readConfigFromBackup() {
    state.cPwr = 16;
    state.cSf = DR_SF7;
    return false;
  }
  
  bool storeConfigToBackup() {
  
  }

  void clearBackup() {
    
  }

#endif
