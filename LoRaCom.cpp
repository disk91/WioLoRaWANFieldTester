#include <Arduino.h>
#include <lmic.h>
#include <hal/hal.h>
#include "keys.h"
#include "config.h"
#include "testeur.h"

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
boolean canSend() {
    if ( nextPossibleSendMs() > 0 ) {
      return false;
    }
    return true;
}

 

void do_send(uint8_t * data, uint8_t sz, _dr_configured_t dr, uint8_t pwr, bool acked, uint8_t retries ) {
    if ( ! canSend() ) {
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
        lmic_tx_error_t err = LMIC_setTxData2(1, data, sz, ((acked)?1:0));
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
    //Serial.print(os_getTime());
    //Serial.print(": ");
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
            break;
        case EV_JOINED:
            LOGLN((F("EV_JOINED")));
            // Disable link check validation (automatically enabled
            // during join, but because slow data rates change max TX
            // size, we don't use it in this example.
            LMIC_setLinkCheckMode(0);

            // Update state
            state.cState = JOINED;
            
            break;
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_RFU1:
        ||     Serial.println(F("EV_RFU1"));
        ||     break;
        */
        case EV_JOIN_FAILED:
            LOGLN((F("EV_JOIN_FAILED")));
            isTransmitting = false;
            break;
        case EV_REJOIN_FAILED:
            LOGLN((F("EV_REJOIN_FAILED")));
            isTransmitting = false;
            break;
        case EV_TXCOMPLETE:
           
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            Serial.printf("TxCnt : %d\r\n",LMIC.txCnt);
            if ( (LMIC.txrxFlags & TXRX_ACK != 0) || LMIC.dataLen > 0 ) {
              // Transmission confirmed
              addInBuffer(LMIC.rssi, LMIC.snr, LMIC.txCnt, LMIC_getSeqnoUp(), false);
              if (LMIC.dataLen) {
                Serial.print(F("Received "));
                Serial.print(LMIC.dataLen);
                Serial.println(F(" bytes of payload"));
                Serial.printf("Rssi %d\r\n",LMIC.rssi);
                Serial.printf("Snr %d\r\n",LMIC.snr);
                Serial.printf("More Data %d\r\n",LMIC.moreData);
              }
            } else {
              // not acked
              Serial.print(F("Not acked "));
              addInBuffer(0, 0, state.cRetry, LMIC_getSeqnoUp(), true);              
            }
            
            // Schedule next transmission
            //os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            isTransmitting = false;
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
            isTransmitting = false;
            break;

        default:
        //    Serial.print(F("Unknown event: "));
        //    Serial.println((unsigned) ev);
            break;
    }
}
