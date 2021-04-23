#include <Arduino.h>
#include <lmic.h>
#include <hal/hal.h>
#include "keys.h"
#include "config.h"
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

#ifndef __APPEUI
#define __APPEUI {0}
#define __DEVEUI {0}
#define __APPKEY {0}
#endif

// Normal order
static const u1_t PROGMEM _APPEUI[8]= __APPEUI;  
void os_getArtEui (u1_t* buf) { 
  for ( int i = 0 ; i < 8 ; i++ ) {
    buf[7-i] = _APPEUI[i];
  }
}
static const u1_t PROGMEM _DEVEUI[8]= __DEVEUI; 
void os_getDevEui (u1_t* buf) { 
  for ( int i = 0 ; i < 8 ; i++ ) {
    buf[7-i] = _DEVEUI[i];
  }
}

// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from ttnctl can be copied as-is.
static const u1_t PROGMEM APPKEY[16] = __APPKEY;
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}


void loraSetup(void) {

    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    LMIC_setClockError(MAX_CLOCK_ERROR * 20 / 100);
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
    #error "Not Yet implemented, please add the channels"
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


// return in Ms time to wait before a new communication in respect of the Duty Cycle
uint32_t nextPossibleSendMs() {
  #ifdef CFG_eu868
    int prevChnl = LMIC.txChnl;
    int32_t ms = osticks2ms(LMICeu868_nextTx(os_getTime())-os_getTime());
    LMIC.txChnl=prevChnl;
    if ( ms > 0 ) return ms;
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
void do_send(uint8_t port, uint8_t * data, uint8_t sz, _dr_configured_t dr, uint8_t pwr, bool acked, uint8_t retries ) {
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
            countRepeat++;
            if ( state.cState != JOINING && state.cState != EMPTY_DWNLINK ) {
              state.cState = ( countRepeat > 1 )? IN_RPT : IN_TX;
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
            isTransmitting = false;
            break;

        default:
        //    Serial.print(F("Unknown event: "));
        //    Serial.println((unsigned) ev);
            break;
    }
}
