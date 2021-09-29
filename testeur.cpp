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
#include "config.h"
#include "testeur.h"
#include "keys.h"
#include "ui.h"
#include "LoRaCom.h"

state_t state;
void initState() {
  if ( ! readConfig() ) {
    uint8_t  _DEVEUI[8]= __DEVEUI; 
    uint8_t  _APPEUI[8]= __APPEUI;  
    uint8_t  _APPKEY[16] = __APPKEY;

    // Config initial setup
    tst_setPower(MAXPOWER);  
    tst_setSf(SLOWERSF);      
    tst_setRetry(1);
    ui.selected_display = DISPLAY_RSSI_HIST;    
    ui.selected_mode = MODE_MANUAL;
    memcpy(loraConf.deveui, _DEVEUI, 8);
    memcpy(loraConf.appeui, _APPEUI, 8);
    memcpy(loraConf.appkey, _APPKEY,16);
    loraConf.zone = __ZONE;
    storeConfig();
  }
  state.cState = NOT_JOINED;

  state.readPtr = 0;
  state.writePtr = 0;
  state.elements = 0;
  state.hasRefreshed = false;
  state.batVoltage = 0;
  state.batPercent = 0;
}

void addInBuffer(int16_t rssi, int16_t snr, uint8_t retry, uint16_t seq, bool lost) {
  state.seq[state.writePtr] = seq;
  if ( ! lost ) { 
      state.rssi[state.writePtr] = rssi;
      state.snr[state.writePtr] = snr;
      state.retry[state.writePtr] = retry;
      state.hs[state.writePtr] = NODATA;
  } else {
      state.rssi[state.writePtr] = 0;
      state.snr[state.writePtr] = 0;
      state.retry[state.writePtr] = LOSTFRAME;
      state.hs[state.writePtr] = NODATA;
  }
  state.writePtr = (state.writePtr+1) & (MAXBUFFER-1);
  if ( state.writePtr == state.readPtr ) {
    state.readPtr = (state.readPtr+1) & (MAXBUFFER-1);  
  }
  if (state.elements < MAXBUFFER) state.elements++;
}

uint8_t getIndexInBuffer(int i) {
  int t = state.readPtr;
  for ( int k = 0 ; k < i ; k++ ) {
    if ( t == state.writePtr ) return MAXBUFFER;
    t = (t+1) & (MAXBUFFER-1);
  }
  return t;
}

uint8_t getIndexBySeq(int seq) {
  int idx = state.readPtr;
  while ( idx != state.writePtr && state.seq[idx] != seq ) {
    idx = ( idx + 1 ) & (MAXBUFFER-1);
  }
  if ( state.seq[idx] == seq ) return idx;
  else return MAXBUFFER;
}

uint8_t getLastIndexWritten() {
  if ( state.writePtr == 0 ) {
    if ( state.readPtr == 0 ) {
      return MAXBUFFER;
    }
    return MAXBUFFER-1;
  }
  return state.writePtr-1;
}


void tst_setPower(int8_t pwr) {
  if ( pwr < 2 ) pwr = 2;
  if ( loraConf.zone == ZONE_EU868 || loraConf.zone == ZONE_AS923 || loraConf.zone == ZONE_KR920 ) {
    if ( pwr > 16 ) pwr = 16;
    pwr &= 0xFE;
  } else if ( loraConf.zone == ZONE_US915 ) {
    if ( pwr > 20 ) pwr = 20;
  } else {
    LOGLN("Zone not supported for power limit");
    if ( pwr > 16 ) pwr = 16;
  }
  state.cPwr = pwr;
}

void tst_setSf(uint8_t sf) {

  if ( sf < 7 ) sf = 7;
  if ( loraConf.zone == ZONE_EU868 || loraConf.zone == ZONE_AS923 || loraConf.zone == ZONE_KR920 || loraConf.zone == ZONE_IN865 ) {
    if ( sf > 12 ) sf = 12;
  } else if ( loraConf.zone == ZONE_US915 ) {
    if ( sf > 10 ) sf = 10;
  } else {
    LOGLN("Zone not supported for SF limit");
    if ( sf > 10 ) sf = 10;
  }
  state.cSf = sf;
  
}

void tst_setRetry(uint8_t retry) {
  if ( retry > MAX_RETRY ) retry = MAX_RETRY;
  if ( retry < 0 ) retry = 0;
  state.cRetry = retry;
}


uint8_t getCurrentSf() {
  return state.cSf;
}
