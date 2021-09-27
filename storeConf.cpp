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
#include "testeur.h"
#include "ui.h"
#include "LoRaCom.h"

#define MAGIC 0xD154
#define VERSION 0x02

typedef struct {
  uint16_t  magic;
  uint8_t   version;
  int8_t    cPwr;     // Current Power
  uint8_t   cSf;      // Current SF
  uint8_t   cRetry;   // Current Number of retry
  uint8_t   selected_display;
  uint8_t   selected_mode;
  uint8_t   deveui[8];   // device EUI
  uint8_t   appeui[8];   // App EUI
  uint8_t   appkey[16];  // App KEY
  uint8_t   zone;        // Zone EU868... cf loracom.h ZONE_XXX
} Config;


FlashStorage(my_flash_store, Config);

// Load configuration from flash
// return tru if loaded false if default must be set
bool readConfig() {

  Config c = my_flash_store.read();
  if ( c.magic == MAGIC && c.version == VERSION ) {
    state.cPwr = c.cPwr;
    state.cSf = c.cSf;
    state.cRetry = c.cRetry;
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
  memcpy(c.deveui, loraConf.deveui, 8);
  memcpy(c.appeui, loraConf.appeui, 8);
  memcpy(c.appkey, loraConf.appkey,16);
  my_flash_store.write(c);
}
