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

 refresUI();


  
  static long cLoop = 0;

  if ( cLoop > 5*1000 ) {
    cLoop = 0;
    do_send(myFrame, sizeof(myFrame),getCurrentDr(), state.cPwr,true, state.cRetry); 
  }
  
  // put your main code here, to run repeatedly:
  loraLoop();
  delay(10);
  cLoop += 10;
  
}
