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
 */  
#include "lmic.h"
#ifndef __TESTER_H
#define __TESTER_H

#define MAXPOWER    127
#define SLOWERSF    255
#define LOSTFRAME   255
#define NODATA      255

#define MAXBUFFER    32

enum e_state {
  NOT_JOINED    = 1,    // Not connected yet
  JOIN_FAILED   = 2,    // Connection failed
  JOINING       = 3,    // Currenlty running the joining procedure
  JOINED        = 4,    // Joined, waiting for action
  IN_TX         = 5,    // Transmitting 
  IN_RPT        = 6,    // 1st transmission failed, running a repeat
  EMPTY_DWNLINK = 7,    // Pending downlink to retrieve before goint to next point

  UKN = 255
};

typedef struct s_state {
  int8_t    cPwr;     // Current Power
  uint8_t   cSf;      // Current SF
  uint8_t   cRetry;   // Current Number of retry
  e_state   cState;   // Current State (Joined / NotJoined)
  
  int16_t   rssi[MAXBUFFER];   // Rssi history
  int16_t   snr[MAXBUFFER];    // Snr history
  uint8_t   retry[MAXBUFFER]; // Retry applied LOSTFRAME when loss
  uint16_t  seq[MAXBUFFER];   // SeqNum history
  
                                  // Sent in a downlink - see later
  uint16_t  hs[MAXBUFFER];        // number HS receiving
  int16_t   bestRssi[MAXBUFFER];  // best Rssi
  int16_t   worstRssi[MAXBUFFER]; // worst Rssi

  int       readPtr;          // circular buffer
  int       writePtr;
  int       elements;         // numbre of data in buffer
  boolean   hasRefreshed;     // Data has refreshed

} state_t;

extern state_t state;

void initState();
void tst_setPower(int8_t pwr);
void tst_setSf(uint8_t sf);
void tst_setRetry(uint8_t retry);
void addInBuffer(int16_t rssi, int16_t snr, uint8_t retry, uint16_t seq, bool lost);
uint8_t getIndexInBuffer(int i);
uint8_t getLastIndexWritten();
uint8_t getIndexBySeq(int seq);
_dr_configured_t getCurrentDr();

#endif
