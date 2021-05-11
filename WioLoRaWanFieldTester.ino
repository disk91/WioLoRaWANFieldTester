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
#include "Arduino.h"
#include <SPI.h>
#include "LoRaCom.h"
#include <Wire.h>
#include "config.h"
#include "ui.h"
#include "testeur.h"


static uint8_t myFrame[] = {
  0x00, 0x00, 0x00, 0x00     // Temperature 16bit x10 High bit first           0 ..  3
};

static uint8_t emptyFrame[] = {
  0x00
};



void setup() {
  // put your setup code here, to run once:
   #ifdef DEBUG
    Serial.begin(9600);
   #endif
   initState();
   initScreen();
   loraSetup();
}


void loop() {
  static long cTime = 0;

  long sTime = millis();
  bool fireMessage = false;

  refresUI();
  switch ( ui.selected_mode ) {
    case MODE_MANUAL:
      if ( ui.hasClick && canLoRaSend() ) { 
        fireMessage = true;
        ui.hasClick = false;
      }
      break;
    case MODE_AUTO_5MIN:
      if ( cTime >= ( 5 * 60 * 1000 ) ) fireMessage = true;
      break;
    case MODE_AUTO_1MIN:
      if ( cTime >= ( 1 * 60 * 1000 ) ) fireMessage = true;
      break;
    case MODE_MAX_RATE:
      if ( canLoRaSend() ) fireMessage = true;
      break;
  }
  if ( state.cState == EMPTY_DWNLINK && canLoRaSend() ) {
    // clean the downlink queue
    // send messages on port2 : the backend will not proceed port 2.
    do_send(2, emptyFrame, sizeof(emptyFrame),getCurrentDr(), state.cPwr,true, state.cRetry); 
  } else if ( fireMessage ) {
    // send a new test message on port 1, backend will create a downlink with information about network side reception
    cTime = 0;
    do_send(1, myFrame, sizeof(myFrame),getCurrentDr(), state.cPwr,true, state.cRetry); 
  }
  
  loraLoop();

  // Wait for the next loop and update time with overflow consideration
  delay(10);
  long duration = millis() - sTime;
  if ( duration < 0 ) duration = 10;
  cTime += duration;
}
