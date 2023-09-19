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
#ifndef __TESTER_H
#define __TESTER_H

#define MAXPOWER    127
#define SLOWERSF    255
#define LOSTFRAME   255
#define NODATA      255
#define NORSSI      255
#define NOSNR       255

#define MAXBUFFER    32

#define DISCO_NONE    0
#define DISCO_READY   1
#define DISCO_WAIT    2
#define DISCO_TX      3
#define DISCO_PAUSE   4
#define DISCO_END     5

#define DISCO_TIME_MS 40000   // inter-frame time 35s
#define DISCO_FRAMES  10      // number of frames

enum e_state {
  NOT_JOINED    = 1,    // Not connected yet
  JOIN_FAILED   = 2,    // Connection failed
  JOINING       = 3,    // Currently running the joining procedure
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
  uint8_t   eptyLoops;// Number of attempt to empty the downlink queue, to not stuck on that state
  boolean   cnfBack;  // Configuration has been backup
  boolean   hidKey;   // Hide Key field in setup
  
  int16_t   rssi[MAXBUFFER];   // Rssi history
  int16_t   snr[MAXBUFFER];    // Snr history
  uint8_t   retry[MAXBUFFER]; // Retry applied LOSTFRAME when loss
  uint16_t  seq[MAXBUFFER];   // SeqNum history
  
                                  // Sent in a downlink - see later
  uint16_t  hs[MAXBUFFER];        // number HS receiving
  int16_t   bestRssi[MAXBUFFER];  // best Rssi
  int16_t   worstRssi[MAXBUFFER]; // worst Rssi
  uint16_t  minDistance[MAXBUFFER]; // nearest hotspot
  uint16_t  maxDistance[MAXBUFFER]; // farest hotspot

  int       readPtr;          // circular buffer
  int       writePtr;
  int       elements;         // number of data in buffer
  boolean   hasRefreshed;     // Data has refreshed

  uint16_t  batVoltage;         // voltage in mV
  uint8_t   batPercent;       // bat percent
  bool      batOk;            // battery detected
  bool      batUpdated;       // battery value has been updated

  bool      gpsOk;            // true when the GPS has been verified and OK

  // Dicovery stuff
  uint8_t   discoveryState;   // current state for discovery
  uint32_t  lastSendMs;       // duration between two transmission
  uint8_t   totalSent;        // number of transmission made   
  uint64_t  startingPosition; // backup the initial position to repeate it.

} state_t;

extern state_t state;

void initState();
void tst_setPower(int8_t pwr);
void tst_setSf(uint8_t sf);
void tst_setRetry(uint8_t retry);
uint8_t addInBuffer(int16_t rssi, int16_t snr, uint8_t retry, uint16_t seq, bool lost);
uint8_t getIndexInBuffer(int i);
uint8_t getLastIndexWritten();
uint8_t getIndexBySeq(uint16_t seq);
uint8_t getCurrentSf();

void enterDisco();
void leaveDisco();
#endif
