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
#if HWTARGET == LORAE5
#include "LoRaCom.h"
#include "testeur.h"
#include "ui.h"

#define DEFAULT_TIMEOUT 2000
#define PASSTHROUGH_TIMEOUT 15000
#define JOIN_TIMEOUT 12000
#define SEND_TIMEOUT_BASE 7000
#define MAX_RESP_BUF_SZ  64
#define MAX_DOWNLINK_BUF 32

typedef struct {
  char bufOkResp[MAX_RESP_BUF_SZ];
  char bufErrResp[MAX_RESP_BUF_SZ];
  char bufEnding[MAX_RESP_BUF_SZ];
  char bufResponse[2*MAX_RESP_BUF_SZ];
  uint8_t bufDownlink[MAX_DOWNLINK_BUF];
  bool withEndingCondition;
  uint16_t respIndex;
  bool runningCommand;
  uint32_t startTime;
  uint32_t elapsedTime;
  uint32_t maxDuration;
  bool statusCommand;
  bool isJoining;
  bool hasJoined;
  int8_t lastDr;
  int8_t lastPower;
  int8_t lastRetry;
  bool hasAcked;
  bool downlinkPending;
  bool gotDownlink;
  uint32_t lastSendMs;
  uint32_t estimatedDCMs;
  bool (*lineProcessing)(void);
  uint8_t currentSeqId; // simplified
  float lastRssi;
  float lastSnr;
  uint8_t tmpInt8;
} loraE5_t;
loraE5_t loraContext;

bool processATResponse();

/**
 * Execute an AT command with a timeout
 * Search for okResp or errResp to determine is the command is a success or a fail
 * When ending is defined, it search for this sentence to consider end of response
 * Can be executed as a sync or async command.
 * okResp and errResp can use joker char with '*'
 * The okResp / errResp surch is a startsWith operation
 * When lineProcessing function is given, each of the line are transmitted to a custom function for processing, when return true, processing is stopped (like for ending)
 */
bool sendATCommand(const char * cmd, const char * okResp, const char * errResp, const char * ending, uint32_t timeoutMs, bool async, bool (*lineProcessing)(void) ) {
  if ( loraContext.runningCommand ) {
    LOGLN(("LoRa already processing"));
    return false;
  }
  loraContext.runningCommand = true;
  loraContext.startTime = millis();
  loraContext.maxDuration = timeoutMs;
  strcpy(loraContext.bufOkResp,okResp);
  strcpy(loraContext.bufErrResp,errResp);
  if ( ending != NULL && strlen(ending) > 0 ) {
    strcpy(loraContext.bufEnding,ending);
    loraContext.withEndingCondition = true;
  } else {
    loraContext.withEndingCondition = false;
  }
  loraContext.respIndex = 0;
  loraContext.lineProcessing = lineProcessing;
  
  SERIALE5.printf("%s\r\n",cmd);
  LOGLORALN((cmd));
  bool done = false;
  if ( !async ) {
    while ( ! processATResponse() );
    return loraContext.statusCommand;
  }
  return true;
}

// compare str with a ref string and return true when
// str starts with ref. Case sensitive. ref can contain
// a joker char *
bool startsWith(const char * str, const char * ref) {
  if ( strlen(str) >= strlen(ref) ) {
    // possible 
    int i;
    for ( i = 0 ; i < strlen(ref) ; i++ ) {
        if ( ref[i] != '*' && str[i] != ref[i] ) {
                break;
        }
    }
    return ( i == strlen(ref) );
  }
  return false;
}

// search for index of char after the ref string in the str
// return -1 when not found
int indexOf(const char * str, const char * ref) {
  
  int sStr = strlen(str);
  int sRef = strlen(ref);
  int e;
  for ( int d = 0 ; d < (sStr - sRef) ; d++ ) {
    if ( str[d] == ref[0] ) {
      for ( e = 1 ; e < sRef ; e++ ) {
        if ( str[d+e] != ref[e] ) {
          break;
        }
      }
      if ( e == sRef ) {
        return d+e;
      }
    }
  }
  return -1;
 
}


/**
 * Process command response
 * return true when nothing more to be done
 */
bool processATResponse() {

  // nothing to be done
  if ( !loraContext.runningCommand ) return true;

  // manage timeout
  uint32_t duration  = millis() - loraContext.startTime;   // overflow after 50D. risk taken.
  if ( duration > loraContext.maxDuration ) {
    loraContext.runningCommand = false;
    loraContext.statusCommand = false;
    LOGLN(("LoRa timeout"));
    return true;
  }
  // process serial line response
  while ( SERIALE5.available() > 0 ) {
      char c = SERIALE5.read();
      if ( (c == '\0' || c == '\r' || c == '\n' ) ) {
        if ( loraContext.respIndex > 0 ) {
          // process line response
          loraContext.bufResponse[loraContext.respIndex] = '\0';
          LOGLORALN((loraContext.bufResponse));
          int i;
          if ( loraContext.lineProcessing != NULL ) {
            if ( loraContext.lineProcessing() ) {
              loraContext.runningCommand = false;
              loraContext.respIndex = 0;
              return true;
            }
          }
          if ( strlen(loraContext.bufErrResp) > 0 && startsWith(loraContext.bufResponse,loraContext.bufErrResp) ) {
              // Error String found
              if ( ! loraContext.withEndingCondition ) {
                loraContext.runningCommand = false;
              }
              loraContext.statusCommand = false;
              loraContext.respIndex = 0;
              LOGLN(("LoRa Error"));
              return !loraContext.withEndingCondition;
          }
          if ( strlen(loraContext.bufOkResp) > 0 && startsWith(loraContext.bufResponse,loraContext.bufOkResp) ) {
              // Success String found
              if ( ! loraContext.withEndingCondition ) {
                loraContext.runningCommand = false;
              }
              loraContext.statusCommand = true;
              loraContext.respIndex = 0;
              return !loraContext.withEndingCondition;
          }
          if ( loraContext.withEndingCondition && startsWith(loraContext.bufResponse,loraContext.bufEnding) ) {
              // this is the end
              loraContext.runningCommand = false;
              loraContext.respIndex = 0;
              return true;
          }
        }
        loraContext.respIndex = 0;
      } else {
        if ( loraContext.respIndex < 2*MAX_RESP_BUF_SZ ) {
          loraContext.bufResponse[loraContext.respIndex] = c;
          loraContext.respIndex++;    
        } else {
          LOGLN(("Response size overflow"));
          loraContext.respIndex = 0;
        }
      }
  }
  return false;
}



void loraSetup(void) {
  char _cmd[128];
  SERIALE5.begin(9600);
  while(!SERIALE5);
  loraContext.runningCommand = false;
  loraContext.hasJoined = false;
  loraContext.isJoining = false;
  loraContext.lastDr = -1;
  loraContext.lastPower = -100;
  loraContext.lastRetry = -1;
  loraContext.currentSeqId = 0xFF;  // next will be 0.
  loraContext.downlinkPending = false;
  loraContext.gotDownlink = false;
     
  if ( ! sendATCommand("AT","+AT: OK","","",DEFAULT_TIMEOUT,false, NULL) ) {
    // retry
    if ( ! sendATCommand("AT","+AT: OK","","",DEFAULT_TIMEOUT,false, NULL) ) {
      // error message
      LoRaMissing();
      while(1);
    }
  }
  sendATCommand("AT+UART=TIMEOUT,0","+UART: TIMEOUT","","",DEFAULT_TIMEOUT,false, NULL);

  // Setup region
  if ( loraConf.zone == ZONE_EU868 ) {
    sendATCommand("AT+DR=EU868","+DR: EU868","+DR: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=3,867.1,0,5","+CH: 3,8671","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=4,867.3,0,5","+CH: 4,8673","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=5,867.5,0,5","+CH: 5,8675","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=6,867.7,0,5","+CH: 6,8677","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=7,867.9,0,5","+CH: 7,8679","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+LW=DC,OFF","+LW: DC, OFF","+LW: ERR","",DEFAULT_TIMEOUT,false,NULL); // manually managed to avoid conflicts
    sendATCommand("AT+LW=JDC,OFF","+LW: JDC, OFF","+LW: ERR","",DEFAULT_TIMEOUT,false,NULL); // manually managed to avoid conflicts  
  } else if ( loraConf.zone == ZONE_US915 || ( loraConf.zone >= ZONE_US915_1 && loraConf.zone <= ZONE_US915_8 ) ) {
    sendATCommand("AT+DR=US915","+DR: US915","+DR: ERR","",DEFAULT_TIMEOUT,false,NULL);
    int s,e;
    switch ( loraConf.zone ) {
      default:
      case ZONE_US915   : s = 8; e = 15; break;
      case ZONE_US915_1 : s = 0; e = 7; break;
      case ZONE_US915_3 : s = 16; e = 23; break;
      case ZONE_US915_4 : s = 24; e = 31; break;
      case ZONE_US915_5 : s = 32; e = 39; break;
      case ZONE_US915_6 : s = 40; e = 47; break;
      case ZONE_US915_7 : s = 48; e = 55; break;
      case ZONE_US915_8 : s = 56; e = 63; break;
    }

    // unvalidate ch outside of selected subband
    for ( int i=0 ; i < 72 ; i++ ) {
      if ( i < s || i > e ) {
        sprintf(_cmd,"AT+CH=%d,OFF",i);
        sendATCommand(_cmd,"+CH: CH","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);  
      }
    }
  } else if ( loraConf.zone == ZONE_AS923_1 ) {
    /*  According to https://github.com/helium/router
        923.6 MHz
        923.8 MHz
        924.0 MHz
        924.2 MHz
        924.4 MHz
    */
    sendATCommand("AT+DR=AS923","+DR: AS923","+DR: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=0,923.2,0,5","+CH: 0,9232","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=1,923.4,0,5","+CH: 1,9234","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=2,923.6,0,5","+CH: 2,9236","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=3,923.8,0,5","+CH: 3,9238","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=4,924.0,0,5","+CH: 4,9240","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=5,924.2,0,5","+CH: 5,9242","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=6,924.4,0,5","+CH: 6,9244","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+RXWIN2=923.2,DR2","+RXWIN2: 9232","+RXWIN2: ERR","",DEFAULT_TIMEOUT,false,NULL);
  } else if ( loraConf.zone == ZONE_AS923_2 ) {
    sendATCommand("AT+DR=AS923","+DR: AS923","+DR: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=0,921.4,0,5","+CH: 0,9214","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=1,921.6,0,5","+CH: 1,9216","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=2,921.8,0,5","+CH: 2,9218","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=3,922.0,0,5","+CH: 3,9220","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=4,922.2,0,5","+CH: 4,9222","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=5,922.4,0,5","+CH: 5,9224","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=6,922.6,0,5","+CH: 6,9226","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+RXWIN2=921.4,DR2","+RXWIN2: 9214","+RXWIN2: ERR","",DEFAULT_TIMEOUT,false,NULL);
  } else if ( loraConf.zone == ZONE_AS923_3 ) {
    sendATCommand("AT+DR=AS923","+DR: AS923","+DR: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=0,916.6,0,5","+CH: 0,9166","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=1,916.8,0,5","+CH: 1,9168","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=2,917.0,0,5","+CH: 2,9170","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=3,917.2,0,5","+CH: 3,9172","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=4,917.4,0,5","+CH: 4,9174","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=5,917.6,0,5","+CH: 5,9176","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=6,917.8,0,5","+CH: 6,9178","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+RXWIN2=916.6,DR2","+RXWIN2: 9166","+RXWIN2: ERR","",DEFAULT_TIMEOUT,false,NULL);
  } else if ( loraConf.zone == ZONE_AS923_4 ) {
    sendATCommand("AT+DR=AS923","+DR: AS923","+DR: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=0,917.3,0,5","+CH: 0,9173","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=1,917.5,0,5","+CH: 1,9175","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=2,917.7,0,5","+CH: 2,9177","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=3,917.9,0,5","+CH: 3,9179","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=4,918.1,0,5","+CH: 4,9181","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=5,918.3,0,5","+CH: 5,9183","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+CH=6,918.5,0,5","+CH: 6,9185","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+RXWIN2=917.3,DR2","+RXWIN2: 9173","+RXWIN2: ERR","",DEFAULT_TIMEOUT,false,NULL);
  } else if ( loraConf.zone == ZONE_KR920 ) {
    sendATCommand("AT+DR=KR920","+DR: KR920","+DR: ERR","",DEFAULT_TIMEOUT,false,NULL);
  } else if ( loraConf.zone == ZONE_IN865 ) {
    sendATCommand("AT+DR=IN865","+DR: IN865","+DR: ERR","",DEFAULT_TIMEOUT,false,NULL);
  } else if ( loraConf.zone == ZONE_AU915 || ( loraConf.zone >= ZONE_AU915_1 && loraConf.zone <= ZONE_AU915_8 ) ) {
    sendATCommand("AT+DR=AU915","+DR: AU915","+DR: ERR","",DEFAULT_TIMEOUT,false,NULL);  
    int s,e;
    switch ( loraConf.zone ) {
      default:
      case ZONE_AU915_1 : s = 0; e = 7; break;
      case ZONE_AU915_2 : s = 8; e = 15; break;
      case ZONE_AU915_3 : s = 16; e = 23; break;
      case ZONE_AU915_4 : s = 24; e = 31; break;
      case ZONE_AU915_5 : s = 32; e = 39; break;
      case ZONE_AU915   : s = 40; e = 47; break;
      case ZONE_AU915_7 : s = 48; e = 55; break;
      case ZONE_AU915_8 : s = 56; e = 63; break;
    }
    // unvalidate ch outside of selected subband
    for ( int i=0 ; i < 72 ; i++ ) {
      if ( i < s || i > e ) {
        sprintf(_cmd,"AT+CH=%d,OFF",i);
        sendATCommand(_cmd,"+CH: CH","+CH: ERR","",DEFAULT_TIMEOUT,false,NULL);  
      }
    }
  } else {
    LOGLN(("Invalid Zone selected"));
    return;
  }
  sendATCommand("AT+ADR=OFF","+ADR: OFF","+ADR: ON","",DEFAULT_TIMEOUT,false,NULL);

  // Setup Ids
    sprintf(_cmd,"AT+ID=DevEUI,%02X%02X%02X%02X%02X%02X%02X%02X",
      loraConf.deveui[0],
      loraConf.deveui[1],
      loraConf.deveui[2],
      loraConf.deveui[3],
      loraConf.deveui[4],
      loraConf.deveui[5],
      loraConf.deveui[6],
      loraConf.deveui[7]
    );
    sendATCommand(_cmd,"+ID: DevEui","+ID: ERR","",DEFAULT_TIMEOUT,false,NULL);
    
    sprintf(_cmd,"AT+ID=AppEUI,%02X%02X%02X%02X%02X%02X%02X%02X",
      loraConf.appeui[0],
      loraConf.appeui[1],
      loraConf.appeui[2],
      loraConf.appeui[3],
      loraConf.appeui[4],
      loraConf.appeui[5],
      loraConf.appeui[6],
      loraConf.appeui[7]
    );
    sendATCommand(_cmd,"+ID: AppEui","+ID: ERR","",DEFAULT_TIMEOUT,false,NULL);

    sprintf(_cmd,"AT+KEY=APPKEY,%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
      loraConf.appkey[0],
      loraConf.appkey[1],
      loraConf.appkey[2],
      loraConf.appkey[3],
      loraConf.appkey[4],
      loraConf.appkey[5],
      loraConf.appkey[6],
      loraConf.appkey[7],
      loraConf.appkey[8],
      loraConf.appkey[9],
      loraConf.appkey[10],
      loraConf.appkey[11],
      loraConf.appkey[12],
      loraConf.appkey[13],
      loraConf.appkey[14],
      loraConf.appkey[15]
    );
    sendATCommand(_cmd,"+KEY: APPKEY","+KEY: ERR","",DEFAULT_TIMEOUT,false,NULL);
    sendATCommand("AT+MODE=LWOTAA","+MODE: LWOTAA","+MODE: ERR","",DEFAULT_TIMEOUT,false,NULL);    
}

// copy a float number into dst string
bool extractNumber(const char * src, char *dst, int maxSz) {
    int idx = 0;
    for ( idx = 0 ; idx < maxSz-1 ; idx ++ ) {
       if ( src[idx] != ',' && src[idx] != ' ' && src[idx] != '\0' ) {
         dst[idx] = src[idx];
       } else {
         break;
       }
    }
    if ( idx < maxSz-1 ) {
       dst[idx] = '\0';  
       return true;
    }
    return false;
}

// copy a hex string into a table
// returns the hexbyte in dst tab and the number of data in sz
// sz is also the max capacity of dst
// stops when non hexchar found
bool extractHexStr(const char * src, uint8_t * dst, uint8_t * sz) {
   uint8_t tNum = 0;
   uint8_t tSz = 0;
   for ( int i = 0 ; ; i++ ) {
    
     if ( src[i] >= '0' && src[i] <= '9' ) {
        tNum += src[i] - '0';
     } else if ( src[i] >= 'a' && src[i] <= 'f' ) {
        tNum += 10 + src[i] - 'a';
     } else if ( src[i] >= 'A' && src[i] <= 'F' ) {
        tNum += 10 + src[i] - 'A';
     } else break;

     if ( (i & 1) == 0 ) {
       // 1st digit
       tNum *= 16;
     } else {
        // 2nd digit - store
        dst[tSz] = tNum;
        tNum = 0;
        tSz++;
        if (tSz >= *sz) break;
     }
    
   }
   *sz = tSz;
   return true;
}

// estimate DutyCycle duration for a single transmission in ms of pause
uint32_t interFrameDutyCycleEstimate(uint8_t _dr, uint8_t retries) {
   if ( loraConf.zone == ZONE_EU868 ) {
      // EU868
      // 1% based on SF and data size (10 Bytes)
      // @TODO also consider ack
      // @TODO make this more generic considering payload size
      if ( retries == 0 ) retries = 1;
      switch (_dr) {
          case 7:  return retries*6200; 
          case 8:  return retries*11300; 
          case 9:  return retries*20600; 
          case 10: return retries*37100; 
          case 11: return retries*82300; 
          case 12: return retries*148300;
          default:
               LOGLN(("Invalid SF"));
               return 200000;
      }
   } else {
      // No Duty Cycle zones, set a minimum time
      return NONDCZONE_DUTYCYCLE_MS;
   }
}


// estimate Tx duration with AT interface
uint32_t txDurationEstimate(uint8_t _dr) {
   switch (_dr) {
     case 7:  return 1500; 
     case 8:  return 1600; 
     case 9:  return 1800; 
     case 10: return 2000; 
     case 11: return 2500; 
     case 12: return 3000;
     default:
          LOGLN(("Invalid SF"));
          return 2000;
   }
}

// ---------------------------------------------------------------------
// Manage transmission response asynchronously
// example :
// 12:23:29.618 -> AT+CMSGHEX=20591A02324505490C07
// 12:23:29.721 -> +CMSGHEX: Start
// 12:23:29.721 -> +CMSGHEX: Wait ACK
// 12:23:33.559 -> +CMSGHEX: ACK Received
// 12:23:33.592 -> +CMSGHEX: PORT: 2; RX: "3E9999010101"
// 12:23:33.627 -> +CMSGHEX: RXWIN1, RSSI -84, SNR 6.0
// 12:23:33.627 -> +CMSGHEX: Done

bool processTx(void) {
  if ( startsWith(loraContext.bufResponse,"+CMSGHEX: RXWIN*, RSSI") ) {
     int s = indexOf(loraContext.bufResponse,"RSSI ");
     loraContext.lastRssi = 0.0;
     loraContext.lastSnr = 0.0;
     if ( s > 0 ) {
        char sRssi[10];
        if ( extractNumber(&loraContext.bufResponse[s], sRssi,10) ) {
           loraContext.lastRssi = atof(sRssi);
        }
     }
     s = indexOf(loraContext.bufResponse,"SNR ");
     if ( s > 0 ) {
        char sSnr[10];
        if ( extractNumber(&loraContext.bufResponse[s], sSnr,10) ) {
           loraContext.lastSnr = atof(sSnr);
        }
     }
     loraContext.hasAcked = true;
     #ifdef WITH_BEEP
     wioBeep(300);
     #endif
  } else if (startsWith(loraContext.bufResponse,"+CMSGHEX: Done")) {
    loraContext.elapsedTime = millis() - loraContext.startTime;
    if ( loraContext.hasAcked ) {
      uint8_t retries = loraContext.elapsedTime / txDurationEstimate(loraContext.lastDr); // really approximative approach
      //Serial.printf("Add Data for seq(%d) rssi(%d) snr(%d) \r\n",loraContext.currentSeqId,(int16_t)loraContext.lastRssi, (int16_t)loraContext.lastSnr);
      LOGF(("Add Frame %d\r\n",loraContext.currentSeqId));
      addInBuffer((int16_t)loraContext.lastRssi, (int16_t)loraContext.lastSnr, retries, loraContext.currentSeqId, false);
      if ( loraContext.gotDownlink ) {
         uint16_t downlinkSeqId = loraContext.bufDownlink[0];
         int idx = getIndexBySeq(downlinkSeqId);
         LOGF(("Search Frame %d %d\r\n",downlinkSeqId, idx));
         if ( idx != MAXBUFFER ) {
            state.worstRssi[idx]  = loraContext.bufDownlink[1];
            state.worstRssi[idx] -= 200;
            if ( state.worstRssi[idx] > 5 ) state.worstRssi[idx] = 5;
            if ( state.worstRssi[idx] < -145 ) state.worstRssi[idx] = -145;
            state.bestRssi[idx]   = loraContext.bufDownlink[2];
            state.bestRssi[idx]  -= 200;
            if ( state.bestRssi[idx] > 5 ) state.bestRssi[idx] = 5;
            if ( state.bestRssi[idx] < -145 ) state.bestRssi[idx] = -145;
            state.minDistance[idx]  = loraContext.bufDownlink[3];
            state.minDistance[idx] *= 250;
            state.maxDistance[idx]  = loraContext.bufDownlink[4];
            state.maxDistance[idx] *= 250;
            state.hs[idx]         = loraContext.bufDownlink[5];
            if ( state.hs[idx] > 20 ) state.hs[idx] = 20;
            if ( state.hs[idx] < 0 ) state.hs[idx] = 0;

            // Fix the lost frame status ... 
            // if we got a response it was finally not lost
            if ( state.retry[idx] == LOSTFRAME ) {
               state.retry[idx] = loraContext.lastRetry;
            }
         }
         #ifdef WITH_BEEP
         wioBeep(600);
         #endif
      }
      state.hasRefreshed = true;
      if ( ui.selected_mode != MODE_MAX_RATE && loraContext.downlinkPending ) {
          state.cState = EMPTY_DWNLINK; 
      } else {
          state.cState = JOINED;              
      }
      loraContext.estimatedDCMs = interFrameDutyCycleEstimate(loraContext.lastDr, retries);
    } else {
      //Serial.printf("Add Data for seq(%d) rssi(%d) snr(%d) [Lost]\r\n",loraContext.currentSeqId,(int16_t)0, (int16_t)0);
      addInBuffer(0, 0, 0, loraContext.currentSeqId, true);
      state.hasRefreshed = true;
      state.cState = JOINED;
    }
  } else if ( startsWith(loraContext.bufResponse,"+CMSGHEX: FPEND") ) {
    // downlink pending
    loraContext.downlinkPending = true;
  } else if ( startsWith(loraContext.bufResponse,"+CMSGHEX: PORT: *; RX: ") ) {
    // downlink content
    int s = indexOf(loraContext.bufResponse,"PORT: ");
    int port=0;
    if ( s > 0 ) {
       char sPort[10];
       if ( extractNumber(&loraContext.bufResponse[s], sPort,10) ) {
          port = atoi(sPort);
       }
    }
    s = indexOf(loraContext.bufResponse,"RX: \"");
    if ( s > 0 ) {
       uint8_t sz = MAX_DOWNLINK_BUF;
       if ( extractHexStr(&loraContext.bufResponse[s], loraContext.bufDownlink, &sz) ) {
          if ( sz == 6 && port == 2 ) {
            loraContext.gotDownlink = true;
          }
       }
    }
  } else if ( startsWith(loraContext.bufResponse,"+CMSGHEX: Length err") ) {
    // current request is not corresponding to the max frame size
    // this may be due to a LoRaWan additional content like MAC command
    state.cState = EMPTY_DWNLINK; // next time we send a smaller frame
    return true; // even if an error, make it as a success.
  } else {
    // unprocessed lines
  }
  return false;
}


void do_send(uint8_t port, uint8_t * data, uint8_t sz, uint8_t _dr, uint8_t pwr, bool acked, uint8_t retries ) {
  char _cmd[128];
  if ( loraContext.lastPower != pwr ) {
    // set power (E5 automatically set to max if higher than max allowed)
    sprintf(_cmd,"AT+POWER=%d",pwr);
    if ( sendATCommand(_cmd,"+POWER:","+POWER: ERR","",DEFAULT_TIMEOUT,false,NULL) ) {
      loraContext.lastPower = pwr;
    } else {
      LOGLN(("Failed to change Power"));
      return;
    }
  }
  if ( loraContext.lastDr != _dr ) {
    // set dr ( for real dr is not dr but SF)
    boolean retDr = true;
    if ( loraConf.zone == ZONE_EU868 
      || loraConf.zone == ZONE_AS923_1 || loraConf.zone == ZONE_AS923_2 || loraConf.zone == ZONE_AS923_3 || loraConf.zone == ZONE_AS923_4
      || loraConf.zone == ZONE_KR920 || loraConf.zone == ZONE_IN865 || loraConf.zone == ZONE_AU915 || ( loraConf.zone >= ZONE_AU915_1 && loraConf.zone <= ZONE_AU915_8 )) {
      // DR0 - SF12 / DR5 - SF7
      switch (_dr) {
        case 7:
             retDr = sendATCommand("AT+DR=DR5","+DR: ***** DR5","+DR: ERR","",DEFAULT_TIMEOUT,false,NULL);
             break;
        case 8:
             retDr = sendATCommand("AT+DR=DR4","+DR: ***** DR4","+DR: ERR","",DEFAULT_TIMEOUT,false,NULL);
             break;
        case 9:
             retDr = sendATCommand("AT+DR=DR3","+DR: ***** DR3","+DR: ERR","",DEFAULT_TIMEOUT,false,NULL);
             break;
        case 10:
             retDr = sendATCommand("AT+DR=DR2","+DR: ***** DR2","+DR: ERR","",DEFAULT_TIMEOUT,false,NULL);
             break;
        case 11:
             retDr = sendATCommand("AT+DR=DR1","+DR: ***** DR1","+DR: ERR","",DEFAULT_TIMEOUT,false,NULL);
             break;
        case 12:
             retDr = sendATCommand("AT+DR=DR0","+DR: ***** DR0","+DR: ERR","",DEFAULT_TIMEOUT,false,NULL);
             break;
        default:
             LOGLN(("Invalid SF"));
             return;
      }
    } else if ( loraConf.zone == ZONE_US915 || (loraConf.zone >= ZONE_US915_1 && loraConf.zone <= ZONE_US915_8 ) ) {
      // DR0 - SF10 / DR3 - SF7
      switch (_dr) {
        case 7:
             retDr = sendATCommand("AT+DR=DR3","+DR: ***** DR3","+DR: ERR","",DEFAULT_TIMEOUT,false,NULL);
             break;
        case 8:
             retDr = sendATCommand("AT+DR=DR2","+DR: ***** DR2","+DR: ERR","",DEFAULT_TIMEOUT,false,NULL);
             break;
        case 9:
             retDr = sendATCommand("AT+DR=DR1","+DR: ***** DR1","+DR: ERR","",DEFAULT_TIMEOUT,false,NULL);
             break;
        case 10:
             retDr = sendATCommand("AT+DR=DR0","+DR: ***** DR0","+DR: ERR","",DEFAULT_TIMEOUT,false,NULL);
             break;
        default:
             LOGLN(("Invalid SF"));
             return;
      }
    } else {
      retDr = false;
    }
    if ( ! retDr ) {
           LOGLN(("Failed to change SF"));
           return;       
    } else {
      loraContext.lastDr = _dr;
    }
  }
  if ( loraContext.lastRetry != retries ) {
    // set retries
    sprintf(_cmd,"AT+RETRY=%d",retries+1);
    if ( sendATCommand(_cmd,"+RETRY:","+RETRY: ERR","",DEFAULT_TIMEOUT,false,NULL) ) {
      loraContext.lastRetry = retries;
    } else {
      LOGLN(("Failed to change Retry"));
      return;
    }
  }

  // Set Pport
  sprintf(_cmd,"AT+PORT=%d",port);
  if ( !sendATCommand(_cmd,"+PORT:","+PORT: ERR","",DEFAULT_TIMEOUT,false,NULL) ) {
     LOGLN(("Invalid Port"));
     return;
  }

  
  loraContext.lastSendMs = millis();
  if ( ! loraContext.hasJoined ) {
    // we first need to join the network
    // make it simple, the first frame will be lost during join
    // 1% based on SF and data size (24 Bytes)
    // @TODO also consider ack
    if ( loraConf.zone == ZONE_EU868 ) {
      switch (_dr) {
        case 7:  loraContext.estimatedDCMs = 8200;  break;
        case 8:  loraContext.estimatedDCMs = 14400; break;
        case 9:  loraContext.estimatedDCMs = 26700; break;
        case 10: loraContext.estimatedDCMs = 49400; break;
        case 11: loraContext.estimatedDCMs = 106900; break;
        case 12: loraContext.estimatedDCMs = 197400; break;
        default:
             LOGLN(("Invalid SF"));
             return;
      }
   } else {
      // No Duty Cycle zones, set a minimum time
      loraContext.estimatedDCMs = NONDCZONE_DUTYCYCLE_MS;
   }
    sendATCommand("AT+JOIN","+JOIN: Network joined","+JOIN: Join failed","+JOIN: Done",JOIN_TIMEOUT,true,NULL);
    loraContext.isJoining = true;
    state.cState = JOINING;
    loraContext.lastDr = -1; // Apparently, after the Join the DR needs to be reset or the frame size becomes too long for US915 (or it is service stuff ?)
    loraContext.lastPower = -100;
    loraContext.lastRetry = -1;

  } else {
      
    loraContext.estimatedDCMs  = interFrameDutyCycleEstimate(_dr,1);
      
    if (acked) {
      sprintf(_cmd,"AT+CMSGHEX=");
      loraContext.estimatedDCMs *= (retries+1);
    } else {
      sprintf(_cmd,"AT+MSGHEX=");
    }
    int k = strlen(_cmd);
    for ( int i = 0 ; i < sz && k < 115 ; i++ ) {
      sprintf(&_cmd[k],"%02X",data[i]);
      k+=2;
    }
    loraContext.hasAcked = false;
    loraContext.downlinkPending = false;
    loraContext.gotDownlink = false;
    loraContext.currentSeqId = (loraContext.currentSeqId + 1) & 0xFF ;
    state.cState = IN_TX;
    if (acked) {
       sendATCommand(_cmd,"+CMSGHEX: Done","","",SEND_TIMEOUT_BASE*(retries+1),true,processTx);     
    } else {
       sendATCommand(_cmd,"+MSGHEX: Done","","",SEND_TIMEOUT_BASE,true,processTx);           
    }
  }
  
}

bool processPassThrough(void) {
  Serial.println(loraContext.bufResponse);
  return false;
}

void loraLoop(void) {
    // --- LoRa E5 passthrough
    // allow to send AT command from Serial line
    static char _buffer[64];
    static uint8_t _bufferIdx = 0;
    while ( Serial.available() && _bufferIdx < 64 ) {
      _buffer[_bufferIdx] = Serial.read();
      if ( _buffer[_bufferIdx] == '\r' || _buffer[_bufferIdx] == '\n' ) {
        _buffer[_bufferIdx] = '\0';
        if ( _bufferIdx > 1 ) {
           if ( ! sendATCommand(_buffer,"","","",PASSTHROUGH_TIMEOUT,true,processPassThrough)) {
             Serial.println("Busy");
           }
        }
        _bufferIdx=0;
      } else {
        _bufferIdx++;
      }
    }
    if ( _bufferIdx == 64 ) _bufferIdx = 0;
    // ---- end of passthrough
    
    if ( processATResponse() ) {
      // process command ended
      
      // Was a join
      if ( loraContext.isJoining ) {
        loraContext.isJoining = false;
        if ( loraContext.statusCommand ) {
            // Joined
            LOGLN(("Joined network"));
            loraContext.hasJoined = true;
            state.cState = JOINED;
            #ifdef WITH_BEEP
            // Two long beeps for joined network
            wioBeep(600);
            delay(200);
            wioBeep(600);
            #endif
        } else {
            // Failed to join
            LOGLN(("Failed to join network"));
            loraContext.hasJoined = false;
            state.cState = JOIN_FAILED;
        }
      }
   }
}

boolean canLoraSleep(void) {
  
  return !loraContext.runningCommand;
  
}

boolean canLoRaSend(){ 
  
    if ( loraContext.runningCommand || nextPossibleSendMs() > 0 ) {
      return false;
    }
    return true;
    
}


uint32_t nextPossibleSendMs(){
  
  int32_t delta = (loraContext.lastSendMs + loraContext.estimatedDCMs) - millis();
  if ( delta > 0 ) return delta;
  return 0;
  
}


bool loraQuickSetup() {
  SERIALE5.begin(9600);
  uint32_t start = millis();
  while ( !SERIALE5 && (millis() - start) < 2000 );
  loraContext.runningCommand = false;
  if ( ! sendATCommand("AT","+AT: OK","","",DEFAULT_TIMEOUT,false, NULL) ) {
    // retry
    if ( ! sendATCommand("AT","+AT: OK","","",DEFAULT_TIMEOUT,false, NULL) ) {
      return false;
    }
  }
  return true;
}

bool storeOneByte(uint8_t adr, uint8_t v) {
    char _cmd[128];
    sprintf(_cmd,"AT+EEPROM=%02X,%02X", adr, v);
    return sendATCommand(_cmd,"+EEPROM: ","","",DEFAULT_TIMEOUT,false,NULL);     
}


bool processRead(void) {
  if ( startsWith(loraContext.bufResponse,"+EEPROM: **, **") ) {
     int s = indexOf(loraContext.bufResponse,"PROM: ");
     if ( s > 0 ) {
        uint8_t values[1];
        uint8_t sz = 1;
        if ( extractHexStr(&loraContext.bufResponse[s+4], values,&sz) ) {
          loraContext.tmpInt8 = values[0];
          return true;
        }
     }
   }
   return false;
}


bool readOneByte(uint8_t adr, uint8_t * v) {
    char _cmd[128];
    sprintf(_cmd,"AT+EEPROM=%02X",adr);
    if ( sendATCommand(_cmd,"+EEPROM: ","","",DEFAULT_TIMEOUT,false,processRead) ) {
      *v = loraContext.tmpInt8;
      return true;          
    }
    return false;
}


#endif
