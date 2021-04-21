#include "lmic.h"
#ifndef __TESTER_H
#define __TESTER_H

#define MAXPOWER    127
#define SLOWERSF    255
#define LOSTFRAME   255

#define MAXBUFFER    32

enum e_state {
  NOT_JOINED = 1,
  JOIN_FAILED = 2,
  JOINING = 3,
  JOINED = 4,
  IN_TX = 5,
  IN_RPT = 6,

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
  uint16_t  hs[MAXBUFFER];         // number HS receiving
  int16_t   bestRssi[MAXBUFFER];  // best Rssi
  int16_t   worstRssi[MAXBUFFER]; // worst Rssi

  int       readPtr;          // circular buffer
  int       writePtr;
  int       elements;         // numbre of data in buffer
  

} state_t;

extern state_t state;

void initState();
void tst_setPower(int8_t pwr);
void tst_setSf(uint8_t sf);
void tst_setRetry(uint8_t retry);
void addInBuffer(int16_t rssi, int16_t snr, uint8_t retry, uint16_t seq, bool lost);
uint8_t getIndexInBuffer(int i);
uint8_t getLastIndexWritten();
_dr_configured_t getCurrentDr();

#endif
