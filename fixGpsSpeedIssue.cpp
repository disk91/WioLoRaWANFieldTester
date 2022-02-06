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

#include "Arduino.h"
#include "config.h"
#if HWTARGET == LORAE5

#include "TFT_eSPI.h"
#include "testeur.h"
#include "LoRaCom.h"
#include "ui.h"
#include "gps.h"

#define FS9         &FreeSerif9pt7b
#define GFXFF       1
#define TFT_GRAY    0b1010010100010000

#define LINEHEIGHT  20


// ---------------------------------------------------------------
// Custom SoftSerial to support WRITE at 115200bps

uint32_t reg_mask;
uint32_t inv_mask;
volatile PORT_OUT_Type * reg;
double dbit_delay_nano = (1/115200.0)*1000000000;
#define portOutputReg(port)   ( &(port->OUT) )
void SetupFastSerial(uint8_t tx) {
  digitalWrite(tx, HIGH);
  pinMode(tx, OUTPUT);
  reg_mask = digitalPinToBitMask(tx);
  inv_mask = ~digitalPinToBitMask(tx);
  PortGroup * port = digitalPinToPort(tx);
  reg = portOutputReg(port);
}


size_t WriteFastSerial(uint8_t b)
{
  uint32_t ibit_delay_nano = 8680; //(uint32_t) dbit_delay_nano;

  uint32_t start = micros();
  uint32_t stop = start + (ibit_delay_nano)/1000; 
  // Write the start bit
  reg->reg &= inv_mask;
  delayMicroseconds(8);
  //while ( micros() < stop ); 

    if (b & 1) // choose bit
      reg->reg |= reg_mask; // send 1
    else
      reg->reg &= inv_mask; // send 0
    delayMicroseconds(8);
    b >>= 1;

    if (b & 1) // choose bit
      reg->reg |= reg_mask; // send 1
    else
      reg->reg &= inv_mask; // send 0
    delayMicroseconds(9);
    b >>= 1;

    if (b & 1) // choose bit
      reg->reg |= reg_mask; // send 1
    else
      reg->reg &= inv_mask; // send 0
    delayMicroseconds(8);
    b >>= 1;

    if (b & 1) // choose bit
      reg->reg |= reg_mask; // send 1
    else
      reg->reg &= inv_mask; // send 0
    delayMicroseconds(9);
    b >>= 1;

    if (b & 1) // choose bit
      reg->reg |= reg_mask; // send 1
    else
      reg->reg &= inv_mask; // send 0
    delayMicroseconds(8);
    b >>= 1;

    if (b & 1) // choose bit
      reg->reg |= reg_mask; // send 1
    else
      reg->reg &= inv_mask; // send 0
    delayMicroseconds(9);
    b >>= 1;

    if (b & 1) // choose bit
      reg->reg |= reg_mask; // send 1
    else
      reg->reg &= inv_mask; // send 0
    delayMicroseconds(8);
    b >>= 1;

    if (b & 1) // choose bit
      reg->reg |= reg_mask; // send 1
    else
      reg->reg &= inv_mask; // send 0
    delayMicroseconds(8);
    b >>= 1;

    // restore pin to natural state
    reg->reg |= reg_mask;
    ibit_delay_nano += ibit_delay_nano;
    delayMicroseconds(10); 
    return 1;
}

// ---------------------------------------------------------------
/*
boolean gpsLoop() {
  
  char c = GPS.read();  
  if (GPS.newNMEAreceived()) {
    // Since this function change internal library var,
    // avoid multiple call, just once and use results later
    Serial.println(GPS.lastNMEA());
    if (GPS.parse(GPS.lastNMEA())) {
      return true;
    }
  }
  return false;
  
}
*/

void processLoRaE5GpsFix() {

  char title[128];
  int y = 10;

  // already verified
  if ( state.gpsOk ) return;

  // make sure we have a E5 board
  if ( ! loraQuickSetup() ) return;
   
  tft.setTextColor(TFT_GRAY);
  tft.setFreeFont(FS9);     // Select the orginal small TomThumb font

  sprintf(title,"Verifying Gps module");
  tft.drawString(title,5, y, GFXFF); y+= LINEHEIGHT; 

  delay(250);
  gpsQuickInit();

  // try to read a NMEA line for 5 seconds
  uint32_t start = millis();
  boolean found = false;
  while ( ! found && (millis() - start) < 5000 ) {
    found = gpsLoop();
  }

  if ( found ) {
    // the device is already correctly setup
    sprintf(title,"GPS is ready, restarting");
    tft.drawString(title,5, y, GFXFF); y+= LINEHEIGHT; 
    //gpsForceBaud115200(); // only for testing, this setup the GPS to 115200
    
    // save this
    state.gpsOk = true;
    storeConfig();
    delay(1000);
    NVIC_SystemReset();
    
  } else {
    sprintf(title,"Reconfiguration is needed");
    tft.drawString(title,5, y, GFXFF); y+= LINEHEIGHT; 
  }

  // Reconfigure
  
  uint8_t cmd[] = "$PQBAUD,W,9600*4B\r\n";
  SetupFastSerial(2);
  for ( int k = 0 ; k < 2 ; k ++ ) {
    sprintf(title,"Reconfiguration trial %d",k+1);
    tft.drawString(title,5, y, GFXFF); y+= LINEHEIGHT; 
    for ( int i = 0 ; cmd[i] > 0 ; i++) {
      //Serial.print((char)cmd[i]);
      WriteFastSerial(cmd[i]);
    }
    delay(5000);
  }

  sprintf(title,"Reconfiguration done ");
  tft.drawString(title,5, y, GFXFF); y+= LINEHEIGHT; 

  // Verify
  delay(250);
  gpsQuickInit();

  start = millis();
  found = false;
  while ( !found && (millis() - start) < 10000 ) {
    found = gpsLoop();
  }

  if ( found ) {
    // the device is  correctly setup
    sprintf(title,"Device is correctly setup");
    tft.drawString(title,5, y, GFXFF); y+= LINEHEIGHT; 
    delay(2000);
    // save this
    state.gpsOk = true;
    storeConfig();
    NVIC_SystemReset();
  } else {
    sprintf(title,"Reconfiguration failed");
    tft.drawString(title,5, y, GFXFF); y+= LINEHEIGHT; 
    sprintf(title,"Wio restarting to try again");
    tft.drawString(title,5, y, GFXFF); y+= LINEHEIGHT; 
    delay(2000);
    NVIC_SystemReset();
  }
}

#endif
