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

#if HWTARGET == RFM95
  #include <lmic.h>
#endif

// init data to verify display
#ifdef DEBUGDATA
void initDebug() {
 int i = 0;
 for ( int k = 0 ; k < 6 ; k++) { // 30 on 32
   state.rssi[i] = -120; state.snr[i] = -15 ; state.retry[i] = 1 ; state.seq[i] = 100+(6*k)+i ; state.hs[i] = 2 ; state.bestRssi[i] = -80 ; state.worstRssi[i] = -130; state.minDistance[i] = 2500 ; state.maxDistance[i] = 15200;i++;
   state.rssi[i] = 5; state.snr[i] = 15 ; state.retry[i] = 1 ; state.seq[i] = 100+(6*k)+i ; state.hs[i] = NODATA ; state.bestRssi[i] = 0 ; state.worstRssi[i] = 0; state.minDistance[i] = 0 ; state.maxDistance[i] = 0;i++;
   state.rssi[i] = NORSSI; state.snr[i] = NOSNR; state.retry[i] = LOSTFRAME ; state.seq[i] = 100+(6*k)+i ; state.hs[i] = NODATA ; state.bestRssi[i] = 0 ; state.worstRssi[i] = 0; state.minDistance[i] = 0 ; state.maxDistance[i] = 0;i++;
   state.rssi[i] = NORSSI; state.snr[i] = NOSNR; state.retry[i] = 1 ; state.seq[i] = 100+(6*k)+i ; state.hs[i] = 1 ; state.bestRssi[i] = -110 ; state.worstRssi[i] = -110; state.minDistance[i] = 7000 ; state.maxDistance[i] = 7000;i++;
   state.rssi[i] = -80; state.snr[i] = -2; state.retry[i] = 2 ; state.seq[i] = 100+(6*k)+i ; state.hs[i] = 1 ; state.bestRssi[i] = -90 ; state.worstRssi[i] = -90; state.minDistance[i] = 30000 ; state.maxDistance[i] = 30000;i++;
   state.rssi[i] = 15; state.snr[i] = 20 ; state.retry[i] = 10 ; state.seq[i] = 100+(6*k)+i ; state.hs[i] = NODATA ; state.bestRssi[i] = 20 ; state.worstRssi[i] = 20; state.minDistance[i] = 0 ; state.maxDistance[i] = 0;i++;
 }
 state.elements = i;
 state.writePtr = i;
}
#endif

state_t state;
void initState() {
  if ( ! readConfig() ) {
    uint8_t  _DEVEUI[8]= __DEVEUI; 
    uint8_t  _APPEUI[8]= __APPEUI;  
    uint8_t  _APPKEY[16] = __APPKEY;

    // Config initial setup
    #if HWTARGET == RFM95
     #if defined CFG_eu868
       loraConf.zone = ZONE_EU868;
      #elif defined CFG_us915
       loraConf.zone = ZONE_US915;
      #endif
    #else    
      loraConf.zone = __ZONE;
    #endif
    tst_setPower(MAXPOWER);  
    tst_setSf(SLOWERSF);      
    tst_setRetry(0);
    ui.selected_display = DISPLAY_RSSI_HIST;    
    ui.selected_mode = MODE_MANUAL;
    memcpy(loraConf.deveui, _DEVEUI, 8);
    memcpy(loraConf.appeui, _APPEUI, 8);
    memcpy(loraConf.appkey, _APPKEY,16);

    state.cnfBack = false;
    state.hidKey = false;
    state.gpsOk = false;
    
    storeConfig();
  }
  state.cState = NOT_JOINED;

  state.readPtr = 0;
  state.writePtr = 0;
  state.elements = 0;
  state.hasRefreshed = false;
  state.batVoltage = 0;
  state.batPercent = 0;
  state.batUpdated = false;


  #ifdef DEBUGDATA
    initDebug();
  #endif
}

void addInBuffer(int16_t rssi, int16_t snr, uint8_t retry, uint16_t seq, bool lost) {
  state.seq[state.writePtr] = seq;
  if ( ! lost ) { 
      state.rssi[state.writePtr] = rssi;
      state.snr[state.writePtr] = snr;
      state.retry[state.writePtr] = retry;
  } else {
      state.rssi[state.writePtr] = NORSSI;
      state.snr[state.writePtr] = NOSNR;
      state.retry[state.writePtr] = LOSTFRAME;
  }
  state.hs[state.writePtr] = NODATA;
  state.worstRssi[state.writePtr] = NORSSI;
  state.bestRssi[state.writePtr] = NORSSI;
  state.minDistance[state.writePtr] = NODATA;
  state.maxDistance[state.writePtr] = NODATA;

  state.writePtr = (state.writePtr+1) & (MAXBUFFER-1);
  if ( state.elements == MAXBUFFER ) {
    state.readPtr = (state.readPtr+1) & (MAXBUFFER-1);  
  } else {
    state.elements++;
  }
}

uint8_t getIndexInBuffer(int i) {
  int t = state.readPtr;
  if ( i > state.elements ) return MAXBUFFER;
  for ( int k = 0 ; k < i ; k++ ) {
    t = (t+1) & (MAXBUFFER-1);
  }
  return t;
}

uint8_t getIndexBySeq(uint16_t seq) {
  int k = 0;
  int idx = state.readPtr;
  while ( idx < state.elements && state.seq[idx] != seq && k < MAXBUFFER ) {
    idx = ( idx + 1 ) & (MAXBUFFER-1);
    k++;
  }
  if ( state.seq[idx] == seq ) return idx;
  else return MAXBUFFER;
}

uint8_t getLastIndexWritten() {
  if ( state.elements == 0 ) return MAXBUFFER;
  if ( state.writePtr == 0 ) return MAXBUFFER-1;
  return state.writePtr-1;
}


void tst_setPower(int8_t pwr) {
  if ( pwr < 2 ) pwr = 2;
  if ( loraConf.zone == ZONE_EU868 || loraConf.zone == ZONE_KR920 ) {
    if ( pwr == MAXPOWER ) {
      pwr = 14;
    }
    if ( pwr > 16 ) pwr = 16;
    #if HWTARGET == RFM95
    pwr &= 0xFE;
    #endif
  } else if ( loraConf.zone == ZONE_AS923_1 || loraConf.zone == ZONE_AS923_2 ||  loraConf.zone == ZONE_AS923_3 ||  loraConf.zone == ZONE_AS923_4 ) {
    if ( pwr == MAXPOWER ) {
      // just to make sure on init we are limiting to 16dBm as some country in AS923 have this limit
      // need to get the detail for each subzone definition.
      pwr = 16;
    }
    if ( pwr > 22 ) pwr = 22;      
  } else if ( loraConf.zone == ZONE_US915 || loraConf.zone == ZONE_AU915 || loraConf.zone == ZONE_IN865 ) {
    #if HWTARGET == LORAE5
      if ( pwr > 22 ) pwr = 22;
    #elif HWTARGET == RFM95
      if ( pwr > 20 ) pwr = 20;
    #else
      #error "Invalid Target"
    #endif
  } else if ( loraConf.zone == ZONE_LATER || loraConf.zone == ZONE_UNDEFINED ) {
    pwr = MAXPOWER;
  } else {
    LOGLN(("Zone not supported for power limit"));
    if ( pwr > 16 ) pwr = 16;
  }
  state.cPwr = pwr;
}

void tst_setSf(uint8_t sf) {

  if ( sf < 7 ) sf = 7;
  if ( loraConf.zone == ZONE_EU868 
    || loraConf.zone == ZONE_AS923_1 || loraConf.zone == ZONE_AS923_2 ||  loraConf.zone == ZONE_AS923_3 ||  loraConf.zone == ZONE_AS923_4 
    || loraConf.zone == ZONE_KR920 || loraConf.zone == ZONE_IN865 || loraConf.zone == ZONE_AU915 ) {
      if ( sf > 12 ) sf = 12;
  } else if ( loraConf.zone == ZONE_US915 ) {
    if ( sf > 10 ) sf = 10;
  } else if ( loraConf.zone == ZONE_LATER || loraConf.zone == ZONE_UNDEFINED ) {
    sf = SLOWERSF;
  } else {
    LOGLN(("Zone not supported for SF limit"));
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
