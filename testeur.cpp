#include <Arduino.h>
#include <lmic.h>
#include "config.h"
#include "testeur.h"

state_t state;
void initState() {
  if ( ! readConfig() ) {
    tst_setPower(MAXPOWER);  
    tst_setSf(SLOWERSF);      
    tst_setRetry(1);    
  }
  state.cState = NOT_JOINED;

  state.readPtr = 0;
  state.writePtr = 0;
  state.elements = 0;
}

void addInBuffer(int16_t rssi, int16_t snr, uint8_t retry, uint16_t seq, bool lost) {
  state.seq[state.writePtr] = seq;
  if ( ! lost ) { 
      state.rssi[state.writePtr] = rssi;
      state.snr[state.writePtr] = snr;
      state.retry[state.writePtr] = retry;
  } else {
      state.rssi[state.writePtr] = 0;
      state.snr[state.writePtr] = 0;
      state.retry[state.writePtr] = LOSTFRAME;
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


void tst_setPower(int8_t pwr) {
  if ( pwr < 2 ) pwr = 2;
  #if defined CFG_eu868
   if ( pwr > 16 ) pwr = 16;
   pwr &= 0xFE;
  #elif defined CFG_us915
    if ( pwr > 20 ) pwr = 20;
  #else
    #error "Not Yet implemented"
  #endif
  state.cPwr = pwr;
}

void tst_setSf(uint8_t sf) {

  if ( sf < 7 ) sf = 7;
  #if defined CFG_eu868
    if ( sf > 12 ) sf = 12;
  #elif defined CFG_us915
    if ( sf > 10 ) sf = 10;
  #else
    #error "Not Yet implemented"
  #endif
  state.cSf = sf;
  
}

void tst_setRetry(uint8_t retry) {
  if ( retry > TXCONF_ATTEMPTS ) retry = TXCONF_ATTEMPTS;
  if ( retry < 0 ) retry = 0;
  state.cRetry = retry;
}


_dr_configured_t getCurrentDr() {
  switch (state.cSf) {
    case 7:
      return DR_SF7;
    case 8:
      return DR_SF8;
    case 9:
      return DR_SF9;
    case 10:
      return DR_SF10;
    case 11:
      return DR_SF11;
    case 12:
      return DR_SF12;
    default:
      return DR_SF7;
  }
}
