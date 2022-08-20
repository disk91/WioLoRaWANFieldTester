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
 *  
 */   
#include "Arduino.h"
#include <SPI.h>
#include "LoRaCom.h"
#include "config.h"
#include "ui.h"
#include "testeur.h"
#include "gps.h"

#if WITH_WIO_BATTERY_PACK
  #include <SparkFunBQ27441.h>
  const unsigned int BATTERY_CAPACITY = 650; 
#endif


// No more than 10 bytes to suppor DR0 in US915
static uint8_t myFrame[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // GPS position 0 if invalid
  0x00, 0x00,                         // Altitude     
  0x00,                               // HDOP x 10
  0x00,                               // Sats
};

static uint8_t emptyFrame[] = {
  0x00
};

// Try to get battery percent depending on voltage 
// Ok that's empiric mapping but better than nothing :-)
// Used 0.5C curve
// https://forum.pycom.io/topic/6585/read_battery_voltage-voltage-value-to-battery-level/3
uint8_t batteryPercent(uint16_t volt) {
  uint8_t percent = 0;
  if (volt >= 4180 ) {
    percent = 100;
  } else if ( volt >= 4100 ) { percent = 95;
  } else if ( volt >= 4000 ) { percent = 90;
  } else if ( volt >= 3950 ) { percent = 85;
  } else if ( volt >= 3900 ) { percent = 80;
  } else if ( volt >= 3850 ) { percent = 70;
  } else if ( volt >= 3800 ) { percent = 60;
  } else if ( volt >= 3750 ) { percent = 50;
  } else if ( volt >= 3725 ) { percent = 40;
  } else if ( volt >= 3700 ) { percent = 30;
  } else if ( volt >= 3650 ) { percent = 20;
  } else if ( volt >= 3600 ) { percent = 10;
  } else if ( volt >= 3400 ) { percent = 5;
  } else if ( volt >= 3200 ) { percent = 2;
  } else {
    percent = 1;
  }
  return percent;
}

void setup() {

  #if defined SERIALCONFIG || defined DEBUG 
     Serial.begin(9600);
     uint32_t s = millis();
     while ( !Serial && (millis() - s) < 3000); 
  #endif
  #if WITH_WIO_BATTERY_PACK
     if ( lipo.begin() ) {
       state.batOk = true;
       lipo.setCapacity(BATTERY_CAPACITY);
     } else {
       state.batOk = false;
     }
  #endif
  initState();
  initScreen();

  // With some LoRa-E5 we have a problem with the GPS default serial speed
  // So ate first we need to fix that
  #if HWTARGET == LORAE5
    processLoRaE5GpsFix();
  #endif
  displayTitle();
  
  // Specific build to clear the LoRa E5 memory storing the LoRa configuration
  // that allows firwmare update.
  #if defined JUSTCLEAN && HWTARGET == LORAE5 
    loraQuickSetup();
    clearBackup();
    while(1);
  #endif

  // Make sure the LoraWan configuration has been made if not wait for the configuration
  boolean zero = true;
  for ( int i = 0 ; i < 8 ; i++ ) {
    if ( loraConf.deveui[i] != 0 ) zero = false;
  }
  bool first = true;
  bool hasChange = false;
  if ( zero ) {
    // try to restore backup
    if ( readConfigFromBackup() ) {
       storeConfig();
       NVIC_SystemReset();
    }
    while (true) {
      if ( manageConfigScreen(true, first || hasChange, false ) ){
        NVIC_SystemReset();
      }
      first = false;
      hasChange = processLoRaConfig();
    }
  } else if ( digitalRead(WIO_5S_PRESS) == LOW ) {
    while ( digitalRead(WIO_5S_PRESS) == LOW );
    manageConfigScreen(false,true, false);
    NVIC_SystemReset();
  }

  #if HWTARGET == LORAE5
  if ( loraConf.zone == ZONE_LATER ) {
    loraConf.zone = ZONE_UNDEFINED;
    manageConfigScreen(false,true,true);
    NVIC_SystemReset();
  }
  #endif

  gpsSetup();
  displaySplash();
  loraSetup();
  // at this point configuration is ready
  if ( ! state.cnfBack ) {
     // backup config
     if ( storeConfigToBackup() ) {
        storeConfig(); // update backup status
     }
  }


  clearScreen();
  screenSetup();
  analogReference(AR_INTERNAL2V23);

}


void loop(void) {

  static unsigned long cTime = 0;
  static unsigned long batUpdateTime = 0;
  static unsigned long noMovementCounter = 0;
  unsigned long sTime;
  bool fireMessage;

  sTime = millis();
  fireMessage = false;
  
  #ifdef WITH_LIPO
    if ( batUpdateTime > 1000 ) {
      #if HWTARGET == RFM95 && WITH_WIO_BATTERY_PACK == 0
        uint32_t v = analogRead(LIPO_ADC);
        v = 2*( 3300 * v ) / 1024;  // should be 2230 ...
        #ifdef LIPO_OFFSET_MV
          v += LIPO_OFFSET_MV;
        #endif
        state.batVoltage = v;
        state.batPercent = batteryPercent(state.batVoltage);
      #elif WITH_WIO_BATTERY_PACK == 1
        if ( state.batOk ) {
          unsigned int soc = lipo.soc();  // Read state-of-charge (%)
          unsigned int volts = lipo.voltage(); // Read battery voltage (mV)
          //unsigned int fullCapacity = lipo.capacity(FULL); // Read full capacity (mAh)
          //unsigned int capacity = lipo.capacity(REMAIN); // Read remaining capacity (mAh)
          //Serial.printf("charge %d / volt : %d / full : %d / remain : %d\r\n",soc,volts,fullCapacity,capacity);
          state.batVoltage = volts;
          state.batPercent = soc;
        }
      #endif
      state.batUpdated = true;
      batUpdateTime = 0;
    }
  #endif
    
  refresUI();  
  switch ( ui.selected_mode ) {
    default:
    case MODE_MANUAL:
      if ( ui.hasClick && canLoRaSend() ) { 
        fireMessage = true;
        ui.hasClick = false;
      }
      break;
    case MODE_AUTO_1H:
      if ( cTime >= ( 60 * 60 * 1000 ) && canLoRaSend() ) fireMessage = true;
      break;
    case MODE_AUTO_15MIN:
      if ( cTime >= ( 15 * 60 * 1000 ) && canLoRaSend() ) fireMessage = true;
      break;
    case MODE_AUTO_5MIN:
      if ( cTime >= ( 5 * 60 * 1000 ) && canLoRaSend() ) fireMessage = true;
      #ifdef WITH_GPS
        if ( noMovementCounter >= MAXNONMOVEMENT_DURATION_MS ) {
          ui.selected_mode = MODE_AUTO_15MIN;
          refreshMode();
        }
      #endif
      break;
    case MODE_AUTO_1MIN:
      if ( cTime >= ( 1 * 60 * 1000 ) && canLoRaSend() ) fireMessage = true;
      #ifdef WITH_GPS
        if ( noMovementCounter >= MAXNONMOVEMENT_DURATION_MS ) {
          ui.selected_mode = MODE_AUTO_15MIN;
          refreshMode();
        }
      #endif
      break;
    case MODE_MAX_RATE:
      #ifdef WITH_GPS
        // join on start
        if ( ( state.cState == NOT_JOINED || state.cState == JOIN_FAILED ) && canLoRaSend() ) fireMessage = true;
        
        // in case no GPS, we switch to 60s minimum
        // for indoor test, manual mode can be use
        if ( ! gps.isReady || ! gpsQualityIsGoodEnough() ) {
           if ( cTime >= ( 1 * 60 * 1000 ) && canLoRaSend() ) fireMessage = true;
        } else {
           // check distance with the previous sent position
           // under 50 meters we send messages on every minutes.
           #ifndef NODISTANCECHECK
             if ( gpsEstimateDistance() > 50 && canLoRaSend() ) fireMessage = true;
             else if ( cTime >= ( 1 * 60 * 1000 ) && canLoRaSend() ) fireMessage = true;
           #else
             // to facilitate debugging
             if ( canLoRaSend() ) fireMessage = true;
           #endif
        }
        if ( noMovementCounter >= MAXNONMOVEMENT_DURATION_MS ) {
          ui.selected_mode = MODE_AUTO_15MIN;
          refreshMode();
        }
      #else
        if ( canLoRaSend() ) fireMessage = true;
      #endif
      break;
  }

  if ( ui.selected_display == DISPLAY_DISCO ) {
    // send a SF10, max power message on every 30 seconds for 5 minutes
    switch ( state.discoveryState ) {
      case DISCO_READY:
        // wait for start
        //nothing to do
        break;
      case DISCO_WAIT:
        // wait for the end of the duty cycle
        if ( canLoRaSend() && gps.isReady && gpsQualityIsGoodEnough() ) {
          state.startingPosition = gpsEncodePosition48b();
          state.discoveryState = DISCO_TX;
          state.lastSendMs = 0xFFFFFFFF;
          state.totalSent = 0;
          refreshDisco();
        }
        break;
      case DISCO_TX:
        // do transmit
        if ( millis() < state.lastSendMs || ( millis() - state.lastSendMs ) > DISCO_TIME_MS ) {
          state.lastSendMs = millis();
          // we send a message
          myFrame[0] = (state.startingPosition >> 40) & 0xFF;
          myFrame[1] = (state.startingPosition >> 32) & 0xFF;
          myFrame[2] = (state.startingPosition >> 24) & 0xFF;
          myFrame[3] = (state.startingPosition >> 16) & 0xFF;
          myFrame[4] = (state.startingPosition >>  8) & 0xFF;
          myFrame[5] = (state.startingPosition      ) & 0xFF;
          do_send(3, myFrame, 6, 10, state.cPwr, false, 0); // send frame SF10 (works everywhere, no ack, no retry on port 3)
          state.totalSent++;
          if ( state.totalSent >= DISCO_FRAMES ) {
            // end
            state.discoveryState = DISCO_END;
          }
          refreshDisco();
        }
        break;
      case DISCO_END:
        // print QR
        // do nothing, wait for the end
        break;
    }
  } else {
    if ( state.cState == EMPTY_DWNLINK && canLoRaSend() ) {
        // clean the downlink queue
        // send messages on port2 : the backend will not proceed port 2.
        do_send(2, emptyFrame, sizeof(emptyFrame),getCurrentSf(), state.cPwr,true, state.cRetry); 
        state.eptyLoops++;
        if ( state.eptyLoops > MAXDOWNLINKRETRY ) {
          state.cState = JOINED;
          state.eptyLoops = 0;
        }
    } else if ( fireMessage ) {
        // send a new test message on port 1, backend will create a downlink with information about network side reception
        cTime = 0;
        // Fill the frame
        #ifdef WITH_GPS
          if ( gps.isReady && gpsQualityIsGoodEnough() ) {
            gpsBackupPosition();
            uint64_t pos = gpsEncodePosition48b();
            myFrame[0] = (pos >> 40) & 0xFF;
            myFrame[1] = (pos >> 32) & 0xFF;
            myFrame[2] = (pos >> 24) & 0xFF;
            myFrame[3] = (pos >> 16) & 0xFF;
            myFrame[4] = (pos >>  8) & 0xFF;
            myFrame[5] = (pos      ) & 0xFF;
            myFrame[6] = ((gps.altitude+1000) >> 8) & 0xFF;
            myFrame[7] = ((gps.altitude+1000)     ) & 0xFF;
            myFrame[8] = (uint8_t)gps.hdop / 10;
            myFrame[9] = gps.sats;
          } else {
            bzero(myFrame, sizeof(myFrame));
          }
        #else
          bzero(myFrame, sizeof(myFrame));
        #endif
        do_send(1, myFrame, sizeof(myFrame),getCurrentSf(), state.cPwr,true, state.cRetry); 
    }
  }
  loraLoop();
  gpsLoop();
  
  // Wait for the next loop and update time with overflow consideration
  delay(10);
  long duration = millis() - sTime;
  if ( duration < 0 ) duration = 10;
  cTime += duration;
  batUpdateTime += duration;
  
  // Time duration without movement
  if (  ! gps.isReady || ! gpsQualityIsGoodEnough() || gpsEstimateDistance() < 50 ) {
    if ( noMovementCounter < MAXNONMOVEMENT_DURATION_MS ) {
       noMovementCounter += duration; 
    }
  } else {
    noMovementCounter = 0;
  }


}
