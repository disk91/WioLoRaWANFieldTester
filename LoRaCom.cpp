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
#include "LoRaCom.h"
#include "testeur.h"
#if HWTARGET == RFM95

#include <lmic.h>
#include <hal/hal.h>
#include "testeur.h"
#include "ui.h"

//static osjob_t sendjob;
static boolean isTransmitting;


// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = RFM95_NSS_PIN,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = RFM95_RST_PIN,
    .dio = {RFM95_DIO_0, RFM95_DIO_1, LMIC_UNUSED_PIN},
};

_dr_configured_t getCurrentDr() {
  switch (getCurrentSf()) {
    case 7:
      return DR_SF7;
    case 8:
      return DR_SF8;
    case 9:
      return DR_SF9;
    case 10:
      return DR_SF10;
#ifdef CFG_eu868
    case 11:
      return DR_SF11;
    case 12:
      return DR_SF12;
#endif
    default:
      return DR_SF7;
  }
}

// Normal order
void os_getArtEui (u1_t* buf) { 
  for ( int i = 0 ; i < 8 ; i++ ) {
    buf[7-i] = loraConf.appeui[i];
  }
}
void os_getDevEui (u1_t* buf) { 
  for ( int i = 0 ; i < 8 ; i++ ) {
    buf[7-i] = loraConf.deveui[i];
  }
}

// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from ttnctl can be copied as-is.
void os_getDevKey (u1_t* buf) {   
  memcpy_P(buf, loraConf.appkey, 16);
}

void loraSetup(void) {

    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    LMIC_setClockError(MAX_CLOCK_ERROR * 10 / 100);
    LMIC_setAdrMode(0);    
    #ifdef CFG_eu868
    LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band 
    LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band 
    LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band 
    LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band 
    LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band 
    LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band 
    LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band 
    LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band 
    LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band   
    LMIC.dn2Dr = SF9; 
    LMIC_setDrTxpow(DR_SF12,14); 
    #elif defined CFG_us915
      LMIC_selectSubBand(1);
    #else
    #error "Not Yet suported, please add the channels"
    #endif

    LMIC_setLinkCheckMode(0); 
    isTransmitting = false;
  
}

void loraLoop(void) {
   os_runloop_once();
}

boolean canLoraSleep(void) {
  return !isTransmitting;
}

static int32_t lastSend = -NONDCZONE_DUTYCYCLE_MS;
// return in Ms time to wait before a new communication in respect of the Duty Cycle
uint32_t nextPossibleSendMs() {
  #ifdef CFG_eu868
    int prevChnl = LMIC.txChnl;
    int32_t ms = osticks2ms(LMICeu868_nextTx(os_getTime())-os_getTime());
    LMIC.txChnl=prevChnl;
    if ( ms > 0 ) return ms;
  #endif
  #ifdef CFG_us915
    // Set a minimum time to US915_DUTYCYCLE_MS milli-seconds
    int32_t ms = NONDCZONE_DUTYCYCLE_MS - (osticks2ms(os_getTime()) - lastSend); 
    if ( ms > 0 ) return ms;
    else lastSend = osticks2ms(os_getTime()) - NONDCZONE_DUTYCYCLE_MS;
  #endif
  return 0;
}


// Make sure we are allowed to transmit in regard of the duty cycle
boolean canLoRaSend() {
    if ( nextPossibleSendMs() > 0 ) {
      return false;
    }
    return true;
}

 
static uint8_t countRepeat = 0;
void do_send(uint8_t port, uint8_t * data, uint8_t sz, uint8_t _dr, uint8_t pwr, bool acked, uint8_t retries ) {

    _dr_configured_t dr;
    switch ( _dr ) {
      #ifndef CFG_us915
      case 12 : dr = DR_SF12; break;
      case 11 : dr = DR_SF11; break;
      #endif
      case 10 : dr = DR_SF10; break;
      case 9 : dr = DR_SF9; break;
      case 8 : dr = DR_SF8; break;
      default : dr = DR_SF7; break;
    }
  
    if ( ! canLoRaSend() ) {
      // Duty cycle limitation
      LOGLN((F("REFUSED_DUTY_CYCLE")));
      return;
    }
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        LOGLN((F("REFUSED_BUSY_SENDING")));
    } else {
        LMIC_setDrTxpow(dr,pwr); 
        // Prepare upstream data transmission at the next possible time.
        countRepeat = 0;
        lmic_tx_error_t err = LMIC_setTxData2(port, data, sz, ((acked)?1:0));
        lastSend = osticks2ms(os_getTime());
        switch ( err ) {
          case LMIC_ERROR_SUCCESS:
              // set number of retry
              LMIC.txCnt = TXCONF_ATTEMPTS - retries;
              isTransmitting = true;
              break;          
          case LMIC_ERROR_TX_BUSY:
              LOGLN((F("ERROR_BUSY")));
              break;
          default:
              LOGLN((F("ERROR")));
              break;
        }
        
    }
    // Next TX is scheduled after TX_COMPLETE event.
}


void onEvent (ev_t ev) {
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            LOGLN((F("EV_SCAN_TIMEOUT")));
            break;
        case EV_BEACON_FOUND:
            LOGLN((F("EV_BEACON_FOUND")));
            break;
        case EV_BEACON_MISSED:
            LOGLN((F("EV_BEACON_MISSED")));
            break;
        case EV_BEACON_TRACKED:
            LOGLN((F("EV_BEACON_TRACKED")));
            break;
        case EV_JOINING:
            LOGLN((F("EV_JOINING")));
            state.cState = JOINING;
            break;
        case EV_JOINED:
            LOGLN((F("EV_JOINED")));
            // Disable link check validation (automatically enabled
            // during join, but because slow data rates change max TX
            // size, we don't use it in this example.
            LMIC_setLinkCheckMode(0);

            // Update state
            state.cState = JOINED;
            countRepeat=0;
            break;
        case EV_JOIN_FAILED:
            LOGLN((F("EV_JOIN_FAILED")));
            isTransmitting = false;
            state.cState = NOT_JOINED;
            break;
        case EV_REJOIN_FAILED:
            LOGLN((F("EV_REJOIN_FAILED")));
            state.cState = NOT_JOINED;
            isTransmitting = false;
            break;
        case EV_TXCOMPLETE: {
            LOGLN(F("TX_COMPLETE"));
            int uplinkSeqId = LMIC_getSeqnoUp();
            uplinkSeqId = (uplinkSeqId == 0)?255:uplinkSeqId-1 & 0xFF;
            if ( (LMIC.txrxFlags & TXRX_ACK != 0) || LMIC.dataLen > 0 || state.cState == JOINING ) {
              boolean isEmptyDownlinkState = ( state.cState == EMPTY_DWNLINK);
                            
              // Transmission confirmed, we have the Rx data
              if ( ! isEmptyDownlinkState ) { 
                addInBuffer(LMIC.rssi, LMIC.snr, countRepeat, uplinkSeqId, false);
                state.hasRefreshed = true;
              }
              if ( ui.selected_mode != MODE_MAX_RATE && ! isEmptyDownlinkState ) {
                state.cState = EMPTY_DWNLINK;
              } else {
                state.cState = JOINED;              
              }
              if (LMIC.dataLen) {
                boolean moreData = false;
                if  (LMIC.dataLen == 6 && LMIC.frame[LMIC.dataBeg-1] == 2) {
                  // This is the expected downlink message
                  int downlinkSeqId = LMIC.frame[LMIC.dataBeg];
                  int idx = getIndexBySeq(downlinkSeqId);
                  if ( idx != MAXBUFFER ) {
                    uint8_t * data = &LMIC.frame[LMIC.dataBeg]; 
                    // valid sequence Id
                    state.worstRssi[idx]  = data[1];
                    state.worstRssi[idx] -= 200;
                    state.bestRssi[idx]   = data[2];
                    state.bestRssi[idx]  -= 200;
                    state.minDistance[idx]  = data[3];
                    state.minDistance[idx] *= 250;
                    state.maxDistance[idx]  = data[4];
                    state.maxDistance[idx] *= 250;
                    state.hs[idx]         = data[5];
                    state.hasRefreshed = true;
                  }
                  if ( LMIC.moreData ) {
                    // we should have pending data to retreive
                    // But in fact we never have moreData set
                    state.cState = EMPTY_DWNLINK;
                  }
                  int lastWrIdx = getLastIndexWritten();
                  if ( lastWrIdx != MAXBUFFER && isEmptyDownlinkState && idx != lastWrIdx ) {
                     state.cState = EMPTY_DWNLINK;
                  }
                }                
              }
            } else {
              // not acked
              LOGLN(F("Not acked"));
              addInBuffer(0, 0, state.cRetry, uplinkSeqId, true);
              state.hasRefreshed = true;
              state.cState = JOINED;
            }            
            isTransmitting = false;
            countRepeat=0;
            }
            break;
        case EV_LOST_TSYNC:
            LOGLN((F("EV_LOST_TSYNC")));
            break;
        case EV_RESET:
            LOGLN((F("EV_RESET")));
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            LOGLN((F("EV_RXCOMPLETE")));
            break;
        case EV_LINK_DEAD:
            isTransmitting = false;
            LOGLN((F("EV_LINK_DEAD")));
            break;
        case EV_LINK_ALIVE:
            LOGLN((F("EV_LINK_ALIVE")));
            break;
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_SCAN_FOUND:
        ||    Serial.println(F("EV_SCAN_FOUND"));
        ||    break;
        */
        case EV_TXSTART:
            LOGLN((F("EV_TXSTART")));
            lastSend = osticks2ms(os_getTime());
            if ( state.cState != NOT_JOINED && state.cState != JOIN_FAILED ) {
              countRepeat++;
              if ( state.cState != JOINING && state.cState != EMPTY_DWNLINK ) {
                state.cState = ( countRepeat > 1 )? IN_RPT : IN_TX;
              }              
            } else {
              state.cState = JOINING;
            }
            break;
        case EV_TXCANCELED:
            isTransmitting = false;
            LOGLN((F("EV_TXCANCELED")));
            break;
        case EV_RXSTART:
            /* do not print anything -- it wrecks timing */
            break;
        case EV_JOIN_TXCOMPLETE:
            LOGLN((F("EV_JOIN_TXCOMPLETE: no JoinAccept")));
            state.cState = JOIN_FAILED;
            lastSend = osticks2ms(os_getTime()); // The LoRaWan stack automatically retry join on US915
            isTransmitting = false;
            break;

        default:
             LOGLN((F("Unknown EVENT")));
        //    Serial.print(F("Unknown event: "));
        //    Serial.println((unsigned) ev);
            break;
    }
}
#endif


/* =======================================================
 * Manage LoRaWan configuration IDs 
 * =======================================================
 */
loraConf_t loraConf;

// Manage the customer device IDs setup over the serial line
uint8_t __charToHex(uint8_t c) {
  if ( c >= 'A' && c <= 'F' ) {
    return 0xa + ( c - 'A' );
  }
  if ( c >= 'a' && c <= 'f' ) {
    return 0xa + ( c - 'a' );
  }
  if ( c >= '0' && c <= '9' ) {
    return ( c - '0' );
  }
  return 0xFF;
}


#define __LCONF_STATE_NONE    0
#define __LCONF_STATE_DEVEUI  1
#define __LCONF_STATE_APPEUI  2
#define __LCONF_STATE_APPKEY  4
#define __LCONF_STATE_ZONE    8
#define __LCONF_STATE_ALL_DONE  15

// return true when config has been partially changed 
bool processLoRaConfig(void) {
  static uint8_t __state=__LCONF_STATE_NONE;
  static uint8_t confStatus=__LCONF_STATE_NONE;
  static uint8_t pos;
  static uint8_t confirmed;
  static char sZone[8];
  bool updated = false;
  #if HWTARGET == RFM95
   // hardcoded
   confStatus |= __LCONF_STATE_ZONE;
   #if defined CFG_eu868 
     loraConf.zone = ZONE_EU868;
   #elif defined CFG_us915
     loraConf.zone = ZONE_US915;
   #endif
  #endif
  while ( SERIALCONFIG.available() ) {
    uint8_t c = SERIALCONFIG.read();
    if ( __state == __LCONF_STATE_NONE ) {
      switch (c) {
        case 'D' : // device EUI
                   __state = __LCONF_STATE_DEVEUI;
                   break;
        case 'A' : // App EUI
                   __state = __LCONF_STATE_APPEUI;
                   break;
        case 'K' : // App KEY
                   __state = __LCONF_STATE_APPKEY;
                   break;
        case 'Z' : // Zone
                   __state = __LCONF_STATE_ZONE;
                   break;
        case '\n': // forget
        case '\r': 
                   break;
        default: // invalid Value
                   SERIALCONFIG.println("KO");
                   break;
      }
      confirmed = 0;
    } else {
      if ( confirmed == 0 ) {
        // here, we are expecting "="
        if ( c == '=' ) {
          confirmed = 1;
          pos = 0;
        } else {
          SERIALCONFIG.println("KO");
          __state = __LCONF_STATE_NONE;
        }
      } else {
        // Now we are processing the Hex Values
        switch (__state) {
          case __LCONF_STATE_DEVEUI: {
            uint8_t v = __charToHex(c);
            if ( v == 0xFF ) goto invalid;
            if ( pos >= 16 ) goto invalid;
            if ( (pos & 1) == 0 ) { // High quartet
              loraConf.deveui[pos/2] = 16*v;
            } else {
              loraConf.deveui[pos/2] += v;
            }
            pos++;
            if ( pos == 16 ) {
              // end of setup
              SERIALCONFIG.print("DEVEUI:");
              for (int i = 0 ; i < 8 ; i++) {
                SERIALCONFIG.printf("%02X",loraConf.deveui[i]);
              }
              SERIALCONFIG.println();
              SERIALCONFIG.println("OK");
              confStatus |= __LCONF_STATE_DEVEUI;
              __state = __LCONF_STATE_NONE;
              updated = true;
            }
          }
          break;
          case __LCONF_STATE_APPEUI: {
            uint8_t v = __charToHex(c);
            if ( v == 0xFF ) goto invalid;
            if ( pos >= 16 ) goto invalid;
            if ( (pos & 1) == 0 ) { // High quartet
              loraConf.appeui[pos/2] = 16*v;
            } else {
              loraConf.appeui[pos/2] += v;
            }
            pos++;
            if ( pos == 16 ) {
              // end of setup
              SERIALCONFIG.print("APPEUI:");
              for (int i = 0 ; i < 8 ; i++) {
                SERIALCONFIG.printf("%02X",loraConf.appeui[i]);
              }
              SERIALCONFIG.println();
              SERIALCONFIG.println("OK");
              confStatus |= __LCONF_STATE_APPEUI;
              __state = __LCONF_STATE_NONE;
              updated = true;
            }
          }
          break;            
          case __LCONF_STATE_APPKEY: {
            uint8_t v = __charToHex(c);
            if ( v == 0xFF ) goto invalid;
            if ( pos >= 32 ) goto invalid;
            if ( (pos & 1) == 0 ) { // High quartet
              loraConf.appkey[pos/2] = 16*v;
            } else {
              loraConf.appkey[pos/2] += v;
            }
            pos++;
            if ( pos == 32 ) {
              // end of setup
              SERIALCONFIG.print("APPKEY:");
              for (int i = 0 ; i < 16 ; i++) {
                SERIALCONFIG.printf("%02X",loraConf.appkey[i]);
              }
              SERIALCONFIG.println();
              SERIALCONFIG.println("OK");
              confStatus |= __LCONF_STATE_APPKEY;
              __state = __LCONF_STATE_NONE;
              updated = true;
            }
          }
          break; 
          case __LCONF_STATE_ZONE: 
            #if HWTARGET == LORAE5
            {
               sZone[pos] = c;  
               pos++;      
               if ( pos == 5  && sZone[0] != 'A' ) {
                  // We should have the zone
                  sZone[5] = '\0';
                  if ( strcmp(sZone,"EU868") == 0 ) {
                    loraConf.zone = ZONE_EU868;
                    SERIALCONFIG.println("ZONE: EU868");
                    SERIALCONFIG.println("OK");
                    confStatus |= __LCONF_STATE_ZONE;
                    updated = true;
                  } else if ( strcmp(sZone,"US915") == 0 ) {
                    loraConf.zone = ZONE_US915;
                    SERIALCONFIG.println("ZONE: US915");
                    SERIALCONFIG.println("OK");
                    confStatus |= __LCONF_STATE_ZONE;
                    updated = true;
                  } else if ( strcmp(sZone,"KR920") == 0 ) {
                    loraConf.zone = ZONE_KR920;
                    SERIALCONFIG.println("ZONE: KR920");
                    SERIALCONFIG.println("OK");
                    confStatus |= __LCONF_STATE_ZONE;
                    updated = true;
                  } else if ( strcmp(sZone,"IN865") == 0 ) {
                    loraConf.zone = ZONE_IN865;
                    SERIALCONFIG.println("ZONE: IN865");
                    SERIALCONFIG.println("OK");
                    confStatus |= __LCONF_STATE_ZONE;
                    updated = true;
                  } else if ( strcmp(sZone,"AU915") == 0 ) {
                    loraConf.zone = ZONE_AU915;
                    SERIALCONFIG.println("ZONE: AU915");
                    SERIALCONFIG.println("OK");
                    confStatus |= __LCONF_STATE_ZONE;
                    updated = true;
                  } else if ( strcmp(sZone,"LATER") == 0 ) {
                    loraConf.zone = ZONE_LATER;
                    SERIALCONFIG.println("ZONE: LATER");
                    SERIALCONFIG.println("OK");
                    confStatus |= __LCONF_STATE_ZONE;
                    updated = true;
                  } else {
                    SERIALCONFIG.println("KO");
                  }
                  __state = __LCONF_STATE_NONE;
               } else if ( pos == 7 && sZone[0] == 'A' ) {
                  // We should have the zone
                  sZone[7] = '\0';
                  if ( strcmp(sZone,"AS923_1") == 0 ) {
                    loraConf.zone = ZONE_AS923_1;
                    SERIALCONFIG.println("ZONE: AS923_1");
                    SERIALCONFIG.println("OK");
                    confStatus |= __LCONF_STATE_ZONE;
                    updated = true;
                  } else if ( strcmp(sZone,"AS923_2") == 0 ) {
                    loraConf.zone = ZONE_AS923_2;
                    SERIALCONFIG.println("ZONE: AS923_2");
                    SERIALCONFIG.println("OK");
                    confStatus |= __LCONF_STATE_ZONE;
                    updated = true;
                  } else if ( strcmp(sZone,"AS923_3") == 0 ) {
                    loraConf.zone = ZONE_AS923_3;
                    SERIALCONFIG.println("ZONE: AS923_3");
                    SERIALCONFIG.println("OK");
                    confStatus |= __LCONF_STATE_ZONE;
                    updated = true;
                  } else if ( strcmp(sZone,"AS923_4") == 0 ) {
                    loraConf.zone = ZONE_AS923_4;
                    SERIALCONFIG.println("ZONE: AS923_4");
                    SERIALCONFIG.println("OK");
                    confStatus |= __LCONF_STATE_ZONE;
                    updated = true;
                  }
               }
            }
            #else
              SERIALCONFIG.println("KO");
              __state = __LCONF_STATE_NONE;
            #endif
          break;
        }
      }
    }  
  }
  // Terminate setting when everything is ok
  if ( confStatus == __LCONF_STATE_ALL_DONE ) {
    // assuming the conf is valid
    state.cnfBack = false;
    if ( loraConf.zone == ZONE_LATER ) {
       state.hidKey = true;
    }

    // Save & reboot
    storeConfig();
    SERIALCONFIG.println("LoRaWan configuration OK");
    NVIC_SystemReset();
  }
  return updated;
invalid:
     SERIALCONFIG.println("KO");
     __state = __LCONF_STATE_NONE;
     return false;
}
