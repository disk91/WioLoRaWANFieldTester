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
#include <math.h>
#include "config.h"
#include "fonts.h"
#include "testeur.h"
#include "TFT_eSPI.h"
#include "ui.h"
#include "LoRaCom.h"
#include "gps.h"

TFT_eSPI tft;
#define X_OFFSET 2
#define Y_OFFSET 0
#define X_SIZE   80
#define Y_SIZE   20
#define R_SIZE    4 
#define BOX_SPACING 2
//                    RRRRRGGGGGGBBBBB
#define TFT_GRAY    0b1010010100010000
#define TFT_GRAY10  0b0100001100001000
#define TFT_GRAY20  0b0010000110000100


#define HIST_X_OFFSET 2
#define HIST_Y_OFFSET 75
#define HIST_X_SIZE 315
#define HIST_X_TXTSIZE X_SIZE-3
#define HIST_Y_SIZE 160
#define HIST_X_BAR_OFFSET 50
#define HIST_X_BAR_SPACE 2

#define MIN_SNR -20
#define MAX_SNR 30
#define MAX_RETRY 8
#define MAX_HS 20
#define MAX_DIST  64000

#define SELECTED_NONE   0
#define SELECTED_POWER  1
#define SELECTED_SF     2
#define SELECTED_RETRY  3

ui_t ui;

void configPending() {
  tft.fillRect(0,120-20,320,40,TFT_WHITE);
  tft.setTextColor(TFT_BLACK);
  tft.setFreeFont(FS9);     // Select the orginal small TomThumb font
  tft.drawString("SETUP CREDENTIALS",70,112, GFXFF);   
}

void initScreen() {
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  pinMode(WIO_KEY_A, INPUT_PULLUP);
  pinMode(WIO_KEY_B, INPUT_PULLUP);
  pinMode(WIO_KEY_C, INPUT_PULLUP);
  pinMode(WIO_5S_UP, INPUT_PULLUP);
  pinMode(WIO_5S_DOWN, INPUT_PULLUP);
  pinMode(WIO_5S_LEFT, INPUT_PULLUP);
  pinMode(WIO_5S_RIGHT, INPUT_PULLUP);
  pinMode(WIO_5S_PRESS, INPUT_PULLUP); 
}

void displayTitle() {
    char title[128];
    uint8_t model = HWTARGET;
    tft.setTextColor(TFT_GRAY);
    tft.setFreeFont(FS9);     // Select the orginal small TomThumb font
    sprintf(title,"Wio LoRaWan Field Tester");
    tft.drawString(title,(320-200)/2, 85, GFXFF);  
    sprintf(title,"Version %s (%s)", VERSION, model==LORAE5 ? "LoRaE5" : "RFM95");
    tft.drawString(title,(320-180)/2, 115, GFXFF);  
    sprintf(title,"WIO_FT_%02X%02X%02X%02X%02X", loraConf.deveui[3],loraConf.deveui[4], loraConf.deveui[5], loraConf.deveui[6], loraConf.deveui[7]);
    tft.drawString(title,(320-160)/2, 180, GFXFF);
}

void displaySplash() {
  // Totally useless so totally mandatory
  tft.fillScreen(TFT_BLACK);
  #ifdef WITH_SPLASH 
    tft.drawRoundRect((320-200)/2,200,200,10,5,TFT_WHITE);
    tft.setTextColor(TFT_GRAY);
    tft.setFreeFont(FS9);     // Select the orginal small TomThumb font
    tft.drawString("disk91.com",(320-90)/2,215, GFXFF);  
    for ( int i = 10 ; i < 100 ; i+=4 ) {
      tft.fillRoundRect((320-200)/2+2,202,((204*i)/100),6,3,TFT_WHITE);
      #if (defined WITH_SPLASH_HELIUM) && ( WITH_SPLASH_HELIUM == 1 )
        draw_splash_helium(HELIUM_XCENTER, (240-100)/2, i);
      #endif
      #if (defined WITH_SPLASH_TTN) && ( WITH_SPLASH_TTN == 1 )
        draw_splash_ttn(TTN_XCENTER, (240-85)/2, i);
      #endif
    }
  #endif
}

void clearScreen() {
    tft.fillScreen(TFT_BLACK);  
}

void screenSetup() {

  if ( ! readConfig() ) {
    ui.selected_display = DISPLAY_RSSI_HIST;    
    ui.selected_mode = MODE_MANUAL;
  }

  ui.selected_menu = SELECTED_NONE;
  ui.displayed_state = UKN;
  ui.transmit_v = 255;
  ui.previous_display = DISPLAY_UNKNONW;
  ui.lastWrId = MAXBUFFER;
  ui.hasClick = false;
  ui.alertMode = false;
  ui.lastGpsUpdateTime = 0;
  
  // draw mask
  refreshPower(); 
  refreshSf();
  refreshRetry();
  refreshState();
  refreshMode();
  refreshLastFrame();
}

/**
 * Call on regular basis by the main loop
 * check the button status to update the user interface
 * 
 */

void refresUI() {

  uint8_t prev_select = ui.selected_menu;
  bool hasAction = false;
  bool forceRefresh = false;
  bool configHasChanged = false;

  #ifdef WITH_LIPO
   if ( refreshLiPo() ) {
    tft.fillScreen(TFT_BLACK);
    refreshPower(); 
    refreshSf();
    refreshRetry();
    refreshState();
    refreshMode();
    refreshLastFrame();
    ui.previous_display = DISPLAY_UNKNONW;
    state.hasRefreshed = true;
   }
   if ( ui.alertMode ) return;
  #endif
  
  if (digitalRead(WIO_KEY_C) == LOW) {
    ui.selected_menu = ( prev_select == SELECTED_POWER )?SELECTED_NONE:SELECTED_POWER;
  } else if (digitalRead(WIO_KEY_B) == LOW) {
    ui.selected_menu = ( prev_select == SELECTED_SF )?SELECTED_NONE:SELECTED_SF;
  } else if (digitalRead(WIO_KEY_A) == LOW) {
    ui.selected_menu = ( prev_select == SELECTED_RETRY )?SELECTED_NONE:SELECTED_RETRY;
  } else if (digitalRead(WIO_5S_UP) == LOW) {
    switch ( ui.selected_menu ) {
      case SELECTED_POWER:
         tst_setPower(state.cPwr+2);
         forceRefresh = true;
         configHasChanged = true;
         break;
      case SELECTED_SF:
         tst_setSf(state.cSf+1);
         configHasChanged = true;
         forceRefresh = true;
         break;
      case SELECTED_RETRY:
         tst_setRetry(state.cRetry+1);
         configHasChanged = true;
         forceRefresh = true;
         break;
       case SELECTED_NONE:
         if ( ui.selected_mode < MODE_MAX ) {
            ui.selected_mode++;
            configHasChanged = true;
            refreshMode();
            hasAction = true;
         }
      default:
         break;   
    }  
    hasAction=true;
  } else if (digitalRead(WIO_5S_DOWN) == LOW) {
    switch ( ui.selected_menu ) {
      case SELECTED_POWER:
         tst_setPower(state.cPwr-2);
         configHasChanged = true;
         forceRefresh = true;
         break;
      case SELECTED_SF:
         tst_setSf(state.cSf-1);
         configHasChanged = true;
         forceRefresh = true;
         break;
      case SELECTED_RETRY:
         tst_setRetry(state.cRetry-1);
         configHasChanged = true;
         forceRefresh = true;
         break;
      case SELECTED_NONE:
         if ( ui.selected_mode > 0 ) {
            ui.selected_mode--;
            configHasChanged = true;
            refreshMode();
            hasAction = true;
         }
      default:
         break;   
    }  
    hasAction=true;
  } else if (digitalRead(WIO_5S_RIGHT) == LOW) {
    configHasChanged = true;
    switch ( ui.selected_display ) {
      case DISPLAY_RSSI_HIST:
         ui.previous_display = ui.selected_display;
         ui.selected_display = DISPLAY_SNR_HIST;
         forceRefresh = true;
         break;
      case DISPLAY_SNR_HIST:
         ui.previous_display = ui.selected_display;
         ui.selected_display = DISPLAY_RETRY_HIST;
         forceRefresh = true;
         break;
      case DISPLAY_RETRY_HIST:
         ui.previous_display = ui.selected_display;
         ui.selected_display = DISPLAY_TXRSSI;
         forceRefresh = true;
         break;
      case DISPLAY_TXRSSI:
         ui.previous_display = ui.selected_display;
         ui.selected_display = DISPLAY_TXHS;
         forceRefresh = true;
         break;
      case DISPLAY_TXHS:
         ui.previous_display = ui.selected_display;
         ui.selected_display = DISPLAY_DISTANCE;
         forceRefresh = true;
         break;
#ifdef WITH_GPS
      case DISPLAY_DISTANCE:
         ui.previous_display = ui.selected_display;
         ui.selected_display = DISPLAY_GPS;
         forceRefresh = true;
         break;
#endif        
      
      default:
         break;   
    }  
    hasAction=true;
  } else if (digitalRead(WIO_5S_LEFT) == LOW) {
    configHasChanged = true;
    switch ( ui.selected_display ) {
#ifdef WITH_GPS
      case DISPLAY_GPS:
         ui.previous_display = ui.selected_display;
         ui.selected_display = DISPLAY_DISTANCE;
         forceRefresh = true;
         break;
#endif       
      case DISPLAY_DISTANCE:
         ui.previous_display = ui.selected_display;
         ui.selected_display = DISPLAY_TXHS;
         forceRefresh = true;
         break;
      case DISPLAY_TXHS:
         ui.previous_display = ui.selected_display;
         ui.selected_display = DISPLAY_TXRSSI;
         forceRefresh = true;
         break;
      case DISPLAY_TXRSSI:
         ui.previous_display = ui.selected_display;
         ui.selected_display = DISPLAY_RETRY_HIST;
         forceRefresh = true;
         break;
      case DISPLAY_RETRY_HIST:
         ui.previous_display = ui.selected_display;
         ui.selected_display = DISPLAY_SNR_HIST;
         forceRefresh = true;
         break;
      case DISPLAY_SNR_HIST:
         ui.previous_display = ui.selected_display;
         ui.selected_display = DISPLAY_RSSI_HIST;
         forceRefresh = true;
         break;
      default:
         break;   
    }  
    hasAction=true;
  } else if (digitalRead(WIO_5S_PRESS) == LOW) {
    if ( ui.selected_mode == MODE_MANUAL ) {
      ui.hasClick = true;
      hasAction = true;
    }
  }

  if ( prev_select != ui.selected_menu || forceRefresh ) {
    if ( prev_select == SELECTED_POWER || ui.selected_menu == SELECTED_POWER ) {
      refreshPower();
    }
    if ( prev_select == SELECTED_SF || ui.selected_menu == SELECTED_SF ) {
      refreshSf();
    }
    if ( prev_select == SELECTED_RETRY || ui.selected_menu == SELECTED_RETRY ) {
      refreshRetry();
    }
    hasAction = true;
  }

  // refresh the Join state part
  refreshState();

  // refresh the GPS part
  #ifdef WITH_GPS
  refreshGps();
  if ( ui.selected_display == DISPLAY_GPS  && gps.updateTime > ui.lastGpsUpdateTime) {
     ui.lastGpsUpdateTime = gps.updateTime;
     refreshGpsDetails();
  }
  #ifdef DEBUGGPS
  if ( ui.selected_display == DISPLAY_GPS && !gps.isReady && gps.rxStuff ) {
     refreshGpsDetails();    
  }
  #else
  if ( ui.selected_display == DISPLAY_GPS && !gps.isReady && gps.updateTime == ui.lastGpsUpdateTime ) {
     // clear display when GPS signal lost
     ui.lastGpsUpdateTime = gps.updateTime+1;
     refreshGpsDetails();    
  }
  #endif

  if ( gps.rxStuff ) {
    gps.rxStuff=false;
  }
  #endif

  
  // refresh the graph history part
  if ( state.hasRefreshed == true
    || ui.previous_display != ui.selected_display 
  ) {
    ui.lastWrId = state.writePtr;
    switch ( ui.selected_display ) {
      case DISPLAY_RSSI_HIST:
        refreshRssiHist();
        break;
      case DISPLAY_SNR_HIST:
        refreshSnrHist();
        break;
      case DISPLAY_RETRY_HIST:
        refreshRetryHist();
        break;
      case DISPLAY_TXRSSI:
        refreshTxRssi();
        break;
      case DISPLAY_TXHS:
        refreshTxHs();
        break;
      case DISPLAY_DISTANCE:
        refreshDistance();
        break;
      case DISPLAY_GPS:
        refreshGpsDetails();
        break;
    }
    if ( state.hasRefreshed == true ) refreshLastFrame();
    state.hasRefreshed = false;
  }  

  if ( configHasChanged ) {
    storeConfig();
  }

  // avoid re-entreing
  if ( hasAction ) delay(300); 
}

/**
 * Ensure LiPo is not connected to 5V directly
 * Draw LiPo level status
 */
bool refreshLiPo() {
  // memoriser l'etat pour ne pas redraw a chaque fois
  // faire un redraw de tout qd back to normal ...
  
  if ( state.batVoltage > 4700 ) {
    // Over Voltage
     if ( ui.alertMode == false ) {
      ui.alertMode = true;
      tft.fillRect(0,120-20,320,40,TFT_RED);
      tft.setTextColor(TFT_WHITE);
      tft.setFreeFont(FS9);     // Select the orginal small TomThumb font
      tft.drawString("SWITCH LiPo IN CHARGE MODE",25,112, GFXFF);  
    }
    return false;
  } else {
    // remove Error message
    if ( ui.alertMode ) {
      ui.alertMode = false;
      return true;
    }
    if ( state.batUpdated ) {
      int xOffset = X_OFFSET+20;
      int yOffset = Y_OFFSET+2*Y_SIZE+5;
      if ( state.batPercent > 0 ) {
        // we have this information (Battery Chassis)
        tft.fillRect(xOffset,yOffset,50 ,10,TFT_BLACK);
        int color = TFT_RED;
        if ( state.batPercent > 50 ) color = TFT_GREEN;
        else if ( state.batPercent > 20 ) color = TFT_ORANGE;
        else if ( state.batPercent < 10 ) {
           delay(100);
           tft.drawRoundRect(xOffset,yOffset,50 ,10,5,TFT_WHITE);
        }
        tft.fillRoundRect(xOffset,yOffset,10 + (40*state.batPercent) / 100 ,10,5,color);
      } else {
        // Display bat status
        if ( state.batVoltage > 3650 ) {
          // green status
          tft.fillRoundRect(xOffset,yOffset,50,10,5,TFT_GREEN);  
        } else if ( state.batVoltage > 3500 ) {
          tft.fillRect(xOffset,yOffset,50 ,10,TFT_BLACK);
          tft.fillRoundRect(xOffset,yOffset,30,10,5,TFT_ORANGE);  
        } else if ( state.batVoltage > 0 ) {
          tft.fillRect(xOffset,yOffset,50 ,10,TFT_BLACK);
          tft.fillRoundRect(xOffset,yOffset,10,10,5,TFT_RED);  
        } else {
          tft.fillRect(xOffset,yOffset,50 ,10,TFT_BLACK);
        }
      }
      tft.drawRoundRect(xOffset,yOffset,50 ,10,5,TFT_WHITE);
      state.batUpdated = false;
    }
  }
  return false;
}

/**
 * Select the way the messages are sent
 * On user action
 * Automatically
 */
void refreshMode() {
  int xOffset = X_OFFSET+3*X_SIZE;
  int yOffset = Y_OFFSET;
  tft.fillRoundRect(xOffset,yOffset,X_SIZE-5,Y_SIZE,R_SIZE,TFT_WHITE);   
  tft.setTextColor(TFT_BLACK);
  tft.setFreeFont(FS9);     // Select the orginal small TomThumb font
  switch ( ui.selected_mode ) {
    case MODE_MANUAL:
      tft.drawString("Manual",xOffset+5,yOffset+3, GFXFF);  
      break;
    case MODE_AUTO_1H:
      tft.drawString("Auto 1h",xOffset+5,yOffset+3, GFXFF);  
      break;
    case MODE_AUTO_15MIN:
      tft.drawString("Auto 15m",xOffset+5,yOffset+3, GFXFF);  
      break;
    case MODE_AUTO_5MIN:
      tft.drawString("Auto 5m",xOffset+5,yOffset+3, GFXFF);  
      break;
    case MODE_AUTO_1MIN:
      tft.drawString("Auto 1m",xOffset+5,yOffset+3, GFXFF);  
      break;
    case MODE_MAX_RATE:
      tft.drawString("Max rate",xOffset+5,yOffset+3, GFXFF);  
      break;
  }
  
}

/**
 * Update the last frame information on top of the screen
 */
void refreshLastFrame() {
  int xOffset = X_OFFSET;
  int yOffset = Y_OFFSET+Y_SIZE+2;
  tft.fillRect(xOffset,yOffset,3*X_SIZE,Y_SIZE,TFT_BLACK);
  tft.drawRoundRect(xOffset,yOffset,3*X_SIZE,Y_SIZE,R_SIZE,TFT_WHITE);
  tft.fillRect(xOffset+X_SIZE-3,yOffset+Y_SIZE,3*X_SIZE,Y_SIZE,TFT_BLACK);
  tft.drawRoundRect(xOffset+X_SIZE-3,yOffset+Y_SIZE,3*X_SIZE,Y_SIZE,R_SIZE,TFT_WHITE);
  int idx = getLastIndexWritten();
  tft.setFreeFont(FS9);
  tft.setTextColor(TFT_WHITE);
  char tmp[100];
  if ( idx != MAXBUFFER ) {
     if ( state.retry[idx] != LOSTFRAME && state.rssi[idx] != NORSSI ) {
       sprintf(tmp,"%04d  %ddBm %ddBm %d rpt",state.seq[idx],state.rssi[idx],state.snr[idx], state.retry[idx]);
     } else {
       sprintf(tmp,"%04d  LOST",state.seq[idx]);
     }
     tft.drawString(tmp,xOffset+3,yOffset+3, GFXFF);     
     if ( state.hs[idx] != NODATA ) {
        sprintf(tmp,">%d / %d< dBm %d hs",state.worstRssi[idx],state.bestRssi[idx],state.hs[idx]);
        tft.drawString(tmp,xOffset+X_SIZE+2,yOffset+Y_SIZE+3, GFXFF);   
     }
  }
  tft.drawRoundRect(xOffset,yOffset,42,Y_SIZE,R_SIZE,TFT_WHITE);
}

/**
 * Update the current state display to see on the corner right / top 
 * the current tester action
 */
void refreshState() {
  int xOffset = X_OFFSET+3*X_SIZE;
  int yOffset = Y_OFFSET+Y_SIZE+2;
  if ( ui.displayed_state != state.cState ) {
    ui.displayed_state = state.cState;
    tft.fillRect(xOffset,yOffset,X_SIZE-BOX_SPACING,Y_SIZE,TFT_BLACK);
    tft.setTextSize(1);
    switch ( ui.displayed_state ) {
      case NOT_JOINED:
      case JOIN_FAILED:
        tft.setTextColor(TFT_RED);
        tft.drawString("Disc",xOffset+3,yOffset+3, GFXFF);   
        break;
      case JOINED:
        tft.setTextColor(TFT_GREEN);
        tft.drawString("Cnx",xOffset+3,yOffset+3, GFXFF);      
        break;
      case JOINING:
        ui.transmit_v = 255; 
        tft.setTextColor(TFT_ORANGE);
        tft.drawString("Join",xOffset+3,yOffset+3, GFXFF); 
        break;
      case IN_TX:
        tft.setTextColor(TFT_GREEN);
        tft.drawString("Tx",xOffset+3,yOffset+3, GFXFF); 
        break;
      case IN_RPT:
        tft.setTextColor(TFT_ORANGE);
        tft.drawString("Tx",xOffset+3,yOffset+3, GFXFF); 
        break;
      case EMPTY_DWNLINK:
        tft.setTextColor(TFT_RED);
        tft.drawString("Dwn",xOffset+3,yOffset+3, GFXFF);   
        break;
    }
  }
   
  // Update duty cycle count-down
  uint8_t next = (nextPossibleSendMs())/1000;
  if ( next != ui.transmit_v ) {
      ui.transmit_v = next;
      if ( ui.transmit_v == 0 ) {
        if ( ui.displayed_state == JOIN_FAILED || ui.displayed_state == NOT_JOINED || ui.displayed_state == JOINING ) {
          tft.fillRoundRect(xOffset+40,yOffset,35,Y_SIZE,R_SIZE,TFT_RED);
          tft.setTextColor(TFT_WHITE);
          tft.setFreeFont(FS9);     // Select the orginal small TomThumb font
          tft.drawString("NA",xOffset+43,yOffset+3, GFXFF);  
        } else {
          if ( ui.displayed_state == IN_TX ) {
            tft.fillRoundRect(xOffset+40,yOffset,35,Y_SIZE,R_SIZE,TFT_RED);
          } else {
            tft.fillRoundRect(xOffset+40,yOffset,35,Y_SIZE,R_SIZE,TFT_GREEN);
          }
        }       
      } else {
        tft.fillRoundRect(xOffset+40,yOffset,35,Y_SIZE,R_SIZE,TFT_RED);
        tft.setTextColor(TFT_WHITE);
        tft.setFreeFont(FS9);     // Select the orginal small TomThumb font
        char sWait[10];
        sprintf(sWait,"%02d",ui.transmit_v); 
        tft.drawString(sWait,xOffset+48,yOffset+3, GFXFF);           
      }
  }
}



void refreshPower() {
  uint16_t color = (ui.selected_menu == SELECTED_POWER)?TFT_WHITE:TFT_GRAY;
  tft.fillRoundRect(X_OFFSET,Y_OFFSET,X_SIZE-BOX_SPACING,Y_SIZE,R_SIZE,color);
  tft.setTextColor(TFT_BLACK);
  tft.setFreeFont(FS9);     // Select the orginal small TomThumb font
  char sPower[10];
  sprintf(sPower,"+%02d dBm",state.cPwr); 
  tft.drawString(sPower,X_OFFSET+5,Y_OFFSET+3, GFXFF);
}

void refreshSf() {
  int xOffset = X_OFFSET+1*X_SIZE;
  uint16_t color = (ui.selected_menu == SELECTED_SF)?TFT_WHITE:TFT_GRAY;
  tft.fillRoundRect(xOffset,Y_OFFSET,X_SIZE-BOX_SPACING,Y_SIZE,R_SIZE,color);
  tft.setTextColor(TFT_BLACK);
  tft.setFreeFont(FS9);     // Select the orginal small TomThumb font
  char sSf[10];
  sprintf(sSf,"SF   %02d",state.cSf); 
  tft.drawString(sSf,xOffset+5,Y_OFFSET+3, GFXFF);
}

void refreshRetry() {
  int xOffset = X_OFFSET+2*X_SIZE;
  uint16_t color = (ui.selected_menu == SELECTED_RETRY)?TFT_WHITE:TFT_GRAY;
  tft.fillRoundRect(xOffset,Y_OFFSET,X_SIZE-BOX_SPACING,Y_SIZE,R_SIZE,color);
  tft.setTextColor(TFT_BLACK);
  tft.setFreeFont(FS9);     // Select the orginal small TomThumb font
  char sRetry[10];
  sprintf(sRetry,"Retry  %01d",state.cRetry); 
  tft.drawString(sRetry,xOffset+5,Y_OFFSET+3, GFXFF);
}


void refreshRssiHist() {

  // No need to refresh everytime
  if ( ui.previous_display != ui.selected_display ) {
    tft.fillRect(HIST_X_OFFSET,HIST_Y_OFFSET-18,HIST_X_TXTSIZE,18,TFT_BLACK);
    tft.fillRect(HIST_X_OFFSET,HIST_Y_OFFSET,HIST_X_SIZE,HIST_Y_SIZE,TFT_BLACK);
    tft.setFreeFont(FF25);    
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Rx Rssi",HIST_X_OFFSET,HIST_Y_OFFSET-18,GFXFF);
    tft.drawRoundRect(HIST_X_OFFSET,HIST_Y_OFFSET,HIST_X_SIZE,HIST_Y_SIZE,R_SIZE,TFT_WHITE);
    ui.previous_display = ui.selected_display;
  }

  // clean the bar
  int xSz = (HIST_X_SIZE - (HIST_X_OFFSET+HIST_X_BAR_OFFSET + MAXBUFFER*HIST_X_BAR_SPACE)) / MAXBUFFER;
  int xOffset = HIST_X_OFFSET+HIST_X_SIZE-xSz-HIST_X_BAR_SPACE;
  for ( int i = 0 ; i < MAXBUFFER ; i++ ) {
     tft.fillRect(xOffset,HIST_Y_OFFSET+1,xSz,154,TFT_BLACK);
     xOffset -= xSz + HIST_X_BAR_SPACE;
  }
  // Redraw lines
  tft.drawLine(HIST_X_OFFSET+2,HIST_Y_OFFSET+10,HIST_X_SIZE-2,HIST_Y_OFFSET+10,TFT_GRAY);  
  for ( int i = 20 ; i < HIST_Y_SIZE ; i+=20 ) {
    if ( i % 40 == 0 ) {
      char sTmp[10];
      sprintf(sTmp,"-%d",i); 
      tft.setFreeFont(FF25);    
      tft.setTextColor(TFT_GRAY);
      tft.drawString(sTmp,HIST_X_OFFSET+5,HIST_Y_OFFSET-5+i,GFXFF);
    }
    tft.drawLine(HIST_X_OFFSET+2,HIST_Y_OFFSET+10+i,HIST_X_SIZE-2,HIST_Y_OFFSET+10+i,TFT_GRAY20);
  }
  xOffset = HIST_X_OFFSET+HIST_X_SIZE-xSz-HIST_X_BAR_SPACE;
  for ( int i = 0 ; i < state.elements ; i++ ) {
     int idx = getIndexInBuffer(state.elements-(i+1));
     if ( idx != MAXBUFFER ) {
       if ( state.rssi[idx] != NORSSI ) {
          int rssi = state.rssi[idx];
          uint16_t color = TFT_GREEN;
          if ( rssi > 8 ) rssi = 8; // avoid drawing over the graph.
          if ( rssi < -125 ) color = TFT_RED;
          else if (rssi < -100 ) color = TFT_ORANGE;
          else if (rssi < -80 ) color = TFT_DARKGREEN;
          if ( rssi < 0 ) {
            tft.fillRect(xOffset,HIST_Y_OFFSET+10,xSz,-rssi,color);
          } else {
            tft.fillRect(xOffset,HIST_Y_OFFSET+10-rssi,xSz,rssi,color);         
          }
       } else {
          tft.drawLine(xOffset+1,HIST_Y_OFFSET+3+10,xOffset+xSz-2,HIST_Y_OFFSET-3+10,TFT_RED);
          tft.drawLine(xOffset+1,HIST_Y_OFFSET-3+10,xOffset+xSz-2,HIST_Y_OFFSET+3+10,TFT_RED);        
       }
     }
     xOffset -= xSz + HIST_X_BAR_SPACE;
  }
  
}
  
void refreshSnrHist() {

  // No need to refresh everytime
  if ( ui.previous_display != ui.selected_display ) {
    tft.fillRect(HIST_X_OFFSET,HIST_Y_OFFSET-18,HIST_X_TXTSIZE,18,TFT_BLACK);
    tft.fillRect(HIST_X_OFFSET,HIST_Y_OFFSET,HIST_X_SIZE,HIST_Y_SIZE,TFT_BLACK);
    tft.setFreeFont(FF25);    
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Rx Snr",HIST_X_OFFSET,HIST_Y_OFFSET-18,GFXFF);
    tft.drawRoundRect(HIST_X_OFFSET,HIST_Y_OFFSET,HIST_X_SIZE,HIST_Y_SIZE,R_SIZE,TFT_WHITE);
    ui.previous_display = ui.selected_display;
  }

  // clean the bar
  int xSz = (HIST_X_SIZE - (HIST_X_OFFSET+HIST_X_BAR_OFFSET + MAXBUFFER*HIST_X_BAR_SPACE)) / MAXBUFFER;
  int xOffset = HIST_X_OFFSET+HIST_X_SIZE-xSz-HIST_X_BAR_SPACE;
  for ( int i = 0 ; i < MAXBUFFER ; i++ ) {
     tft.fillRect(xOffset,HIST_Y_OFFSET+2,xSz,HIST_Y_SIZE-4,TFT_BLACK);
     xOffset -= xSz + HIST_X_BAR_SPACE;
  }
  // Redraw lines
  int yOffset = HIST_Y_OFFSET+HIST_Y_SIZE-10;
  int yStep10 = ((HIST_Y_OFFSET+HIST_Y_SIZE-10) - ( HIST_Y_OFFSET - 5 )) / (( MAX_SNR - MIN_SNR )/10);  // step for 10 SNR
  int value = MIN_SNR;
  for ( int i = 0 ; i < (MAX_SNR-MIN_SNR) ; i+= 10, value+=10 ) {
    int y = yOffset-(yStep10*i)/10;
    if ( i % 10 == 0 ) {
      char sTmp[10];
      sprintf(sTmp,"%d",value); 
      tft.setFreeFont(FF25);    
      tft.setTextColor(TFT_GRAY);
      tft.drawString(sTmp,HIST_X_OFFSET+5,y-15,GFXFF);
    }
    int color = (value==0)?TFT_GRAY:TFT_GRAY20;
    tft.drawLine(HIST_X_OFFSET+2,y,HIST_X_SIZE-2,y,color);
  }
  xOffset = HIST_X_OFFSET+HIST_X_SIZE-xSz-HIST_X_BAR_SPACE;
  int yOffsetZero = yOffset-(-MIN_SNR*yStep10)/10;
  for ( int i = 0 ; i < state.elements ; i++ ) {
     int idx = getIndexInBuffer(state.elements-(i+1));
     if ( idx != MAXBUFFER ) {
        if ( state.snr[idx] != NOSNR ) {
            int snr = state.snr[idx];
            if ( snr < MIN_SNR ) snr = MIN_SNR;
            if ( snr > MAX_SNR - 5 ) snr = MAX_SNR -5; // we had some overflowing, @TODO investigate later
            uint16_t color = TFT_GREEN;
            if ( snr < -10 ) color = TFT_RED;
            else if (snr < 0 ) color = TFT_ORANGE;
            else if (snr < 5 ) color = TFT_DARKGREEN;   
            if ( snr > 0 ) {
               tft.fillRect(xOffset,yOffsetZero-(snr*yStep10)/10,xSz,(snr*yStep10)/10,color);
            } else {
               tft.fillRect(xOffset,yOffsetZero,xSz,-(snr*yStep10)/10,color);
            }
         } else {
            tft.drawLine(xOffset+1,yOffsetZero+3,xOffset+xSz-2,yOffsetZero-3,TFT_RED);
            tft.drawLine(xOffset+1,yOffsetZero-3,xOffset+xSz-2,yOffsetZero+3,TFT_RED);        
         }
     }
     xOffset -= xSz + HIST_X_BAR_SPACE;
  }
  
}

void refreshRetryHist() {

  // No need to refresh everytime
  if ( ui.previous_display != ui.selected_display ) {
    tft.fillRect(HIST_X_OFFSET,HIST_Y_OFFSET-18,HIST_X_TXTSIZE,18,TFT_BLACK);
    tft.fillRect(HIST_X_OFFSET,HIST_Y_OFFSET,HIST_X_SIZE,HIST_Y_SIZE,TFT_BLACK);
    tft.setFreeFont(FF25);    
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Retry",HIST_X_OFFSET,HIST_Y_OFFSET-18,GFXFF);
    tft.drawRoundRect(HIST_X_OFFSET,HIST_Y_OFFSET,HIST_X_SIZE,HIST_Y_SIZE,R_SIZE,TFT_WHITE);
    ui.previous_display = ui.selected_display;
  }

  // clean the bar
  int xSz = (HIST_X_SIZE - (HIST_X_OFFSET+HIST_X_BAR_OFFSET + MAXBUFFER*HIST_X_BAR_SPACE)) / MAXBUFFER;
  int xOffset = HIST_X_OFFSET+HIST_X_SIZE-xSz-HIST_X_BAR_SPACE;
  for ( int i = 0 ; i < MAXBUFFER ; i++ ) {
     tft.fillRect(xOffset,HIST_Y_OFFSET+2,xSz,HIST_Y_SIZE-4,TFT_BLACK);
     xOffset -= xSz + HIST_X_BAR_SPACE;
  }
  // Redraw lines
  int yOffset = HIST_Y_OFFSET+HIST_Y_SIZE-10;
  int yStep = ((HIST_Y_OFFSET+HIST_Y_SIZE-10) - ( HIST_Y_OFFSET - 5 )) / (MAX_RETRY);  
  tft.drawLine(HIST_X_OFFSET+2,yOffset,HIST_X_SIZE-2,yOffset,TFT_GRAY);  
  for ( int i = 1 ; i < MAX_RETRY ; i+= 1 ) {
    int y = yOffset-(yStep*i);
    if ( i % 2 == 0 ) {
      char sTmp[10];
      sprintf(sTmp,"%d",i); 
      tft.setFreeFont(FF25);    
      tft.setTextColor(TFT_GRAY);
      tft.drawString(sTmp,HIST_X_OFFSET+5,y-15,GFXFF);
    }
    tft.drawLine(HIST_X_OFFSET+2,y,HIST_X_SIZE-2,y,TFT_GRAY20);
  }
  xOffset = HIST_X_OFFSET+HIST_X_SIZE-xSz-HIST_X_BAR_SPACE;
  for ( int i = 0 ; i < state.elements ; i++ ) {
     int idx = getIndexInBuffer(state.elements-(i+1));
     if ( idx != MAXBUFFER && state.retry[idx] != LOSTFRAME ) {
        int retry = state.retry[idx];
        if ( retry == 0 ) {
          tft.drawLine(xOffset+1,yOffset+3,xOffset+xSz-2,yOffset-3,TFT_GREEN);
          tft.drawLine(xOffset+1,yOffset-3,xOffset+xSz-2,yOffset+3,TFT_GREEN);        
        } else {
          uint16_t color = TFT_RED;
          if ( retry == 1 ) color = TFT_DARKGREEN;
          else if (retry == 2 ) color = TFT_ORANGE;
          tft.fillRect(xOffset,yOffset-(retry*yStep),xSz,(retry*yStep),color);
        }
     } else {
        tft.drawLine(xOffset+1,yOffset+3,xOffset+xSz-2,yOffset-3,TFT_RED);
        tft.drawLine(xOffset+1,yOffset-3,xOffset+xSz-2,yOffset+3,TFT_RED);        
     }
     xOffset -= xSz + HIST_X_BAR_SPACE;
  }
  
}



void refreshTxRssi() {

  // No need to refresh everytime
  if ( ui.previous_display != ui.selected_display ) {
    tft.fillRect(HIST_X_OFFSET,HIST_Y_OFFSET-18,HIST_X_TXTSIZE,18,TFT_BLACK);
    tft.fillRect(HIST_X_OFFSET,HIST_Y_OFFSET,HIST_X_SIZE,HIST_Y_SIZE,TFT_BLACK);
    tft.setFreeFont(FF25);    
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Tx Rssi",HIST_X_OFFSET,HIST_Y_OFFSET-18,GFXFF);
    tft.drawRoundRect(HIST_X_OFFSET,HIST_Y_OFFSET,HIST_X_SIZE,HIST_Y_SIZE,R_SIZE,TFT_WHITE);
    ui.previous_display = ui.selected_display;
  }

  // clean the bar
  int xSz = (HIST_X_SIZE - (HIST_X_OFFSET+HIST_X_BAR_OFFSET + MAXBUFFER*HIST_X_BAR_SPACE)) / MAXBUFFER;
  int xOffset = HIST_X_OFFSET+HIST_X_SIZE-xSz-HIST_X_BAR_SPACE;
  for ( int i = 0 ; i < MAXBUFFER ; i++ ) {
     tft.fillRect(xOffset,HIST_Y_OFFSET+1,xSz,154,TFT_BLACK);
     xOffset -= xSz + HIST_X_BAR_SPACE;
  }
  // Redraw lines
  tft.drawLine(HIST_X_OFFSET+2,HIST_Y_OFFSET+10,HIST_X_SIZE-2,HIST_Y_OFFSET+10,TFT_GRAY);  
  for ( int i = 20 ; i < HIST_Y_SIZE ; i+=20 ) {
    if ( i % 40 == 0 ) {
      char sTmp[10];
      sprintf(sTmp,"-%d",i); 
      tft.setFreeFont(FF25);    
      tft.setTextColor(TFT_GRAY);
      tft.drawString(sTmp,HIST_X_OFFSET+5,HIST_Y_OFFSET-5+i,GFXFF);
    }
    tft.drawLine(HIST_X_OFFSET+2,HIST_Y_OFFSET+10+i,HIST_X_SIZE-2,HIST_Y_OFFSET+10+i,TFT_GRAY20);
  }
  xOffset = HIST_X_OFFSET+HIST_X_SIZE-xSz-HIST_X_BAR_SPACE;
  for ( int i = 0 ; i < state.elements ; i++ ) {
     int idx = getIndexInBuffer(state.elements-(i+1));
     if ( idx != MAXBUFFER ) {
       if ( state.hs[idx] != NODATA ) {
          int minRssi = state.worstRssi[idx];
          int maxRssi = state.bestRssi[idx];
          uint16_t color = TFT_GREEN;
          if ( maxRssi > 8 ) maxRssi = 8; // avoid drawing over the graph.
          if ( minRssi > 8 ) minRssi = 8;
          
          if ( maxRssi < -125 ) color = TFT_RED;
          else if (maxRssi < -100 ) color = TFT_ORANGE;
          else if (maxRssi < -80 ) color = TFT_DARKGREEN;
  
          if ( minRssi != maxRssi ) {
            tft.fillRect(xOffset,(HIST_Y_OFFSET)+(-maxRssi+10),xSz,(maxRssi-minRssi),color);
          } else {
            tft.fillRect(xOffset,(HIST_Y_OFFSET)+(-minRssi+10)-1,xSz,2,color);
          }
       } else {
          tft.drawLine(xOffset+1,HIST_Y_OFFSET+3+10,xOffset+xSz-2,HIST_Y_OFFSET-3+10,TFT_RED);
          tft.drawLine(xOffset+1,HIST_Y_OFFSET-3+10,xOffset+xSz-2,HIST_Y_OFFSET+3+10,TFT_RED);        
       }
     }
     xOffset -= xSz + HIST_X_BAR_SPACE;
  }
  
}

void refreshTxHs() {
   // No need to refresh everytime
  if ( ui.previous_display != ui.selected_display ) {
    tft.fillRect(HIST_X_OFFSET,HIST_Y_OFFSET-18,HIST_X_TXTSIZE,18,TFT_BLACK);
    tft.fillRect(HIST_X_OFFSET,HIST_Y_OFFSET,HIST_X_SIZE,HIST_Y_SIZE,TFT_BLACK);
    tft.setFreeFont(FF25);    
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Hotspots",HIST_X_OFFSET,HIST_Y_OFFSET-18,GFXFF);
    tft.drawRoundRect(HIST_X_OFFSET,HIST_Y_OFFSET,HIST_X_SIZE,HIST_Y_SIZE,R_SIZE,TFT_WHITE);
    ui.previous_display = ui.selected_display;
  }

  // clean the bar
  int xSz = (HIST_X_SIZE - (HIST_X_OFFSET+HIST_X_BAR_OFFSET + MAXBUFFER*HIST_X_BAR_SPACE)) / MAXBUFFER;
  int xOffset = HIST_X_OFFSET+HIST_X_SIZE-xSz-HIST_X_BAR_SPACE;
  for ( int i = 0 ; i < MAXBUFFER ; i++ ) {
     tft.fillRect(xOffset,HIST_Y_OFFSET+2,xSz,HIST_Y_SIZE-4,TFT_BLACK);
     xOffset -= xSz + HIST_X_BAR_SPACE;
  }
  // Redraw lines
  int yOffset = HIST_Y_OFFSET+HIST_Y_SIZE-10;
  int yStep = ((HIST_Y_OFFSET+HIST_Y_SIZE-10) - ( HIST_Y_OFFSET - 5 )) / (MAX_HS);  
  tft.drawLine(HIST_X_OFFSET+2,yOffset,HIST_X_SIZE-2,yOffset,TFT_GRAY);  
  for ( int i = 1 ; i < MAX_HS ; i+= 1 ) {
    int y = yOffset-(yStep*i);
    if ( i % 5 == 0 ) {
       tft.drawLine(HIST_X_OFFSET+2,y,HIST_X_SIZE-2,y,TFT_GRAY20);
    } else {
       tft.drawLine(HIST_X_OFFSET+2+30,y,HIST_X_SIZE-2-5,y,TFT_GRAY20);
    }
  }
  for ( int i = 5 ; i < MAX_HS ; i+= 5 ) {
    int y = yOffset-(yStep*i);
    char sTmp[10];
    sprintf(sTmp,"%d",i); 
    tft.setFreeFont(FF25);    
    tft.setTextColor(TFT_GRAY);
    tft.drawString(sTmp,HIST_X_OFFSET+5,y-15,GFXFF);
  }
  xOffset = HIST_X_OFFSET+HIST_X_SIZE-xSz-HIST_X_BAR_SPACE;
  for ( int i = 0 ; i < state.elements ; i++ ) {
     int idx = getIndexInBuffer(state.elements-(i+1));
     if ( idx != MAXBUFFER ) {
       if ( state.hs[idx] != NODATA ) {
          int hs = state.hs[idx];
          if ( hs == 0 ) {
            tft.drawLine(xOffset+1,yOffset+3,xOffset+xSz-2,yOffset-3,TFT_GREEN);
            tft.drawLine(xOffset+1,yOffset-3,xOffset+xSz-2,yOffset+3,TFT_GREEN);        
          } else {
            uint16_t color = TFT_GREEN;
            if ( hs == 1 ) color = TFT_RED;
            else if ( hs == 2 ) color = TFT_ORANGE;
            else if ( hs < 4 ) color = TFT_DARKGREEN;
            tft.fillRect(xOffset,yOffset-(hs*yStep),xSz,(hs*yStep),color);
          }
       } else {
          tft.drawLine(xOffset+1,yOffset+3,xOffset+xSz-2,yOffset-3,TFT_RED);
          tft.drawLine(xOffset+1,yOffset-3,xOffset+xSz-2,yOffset+3,TFT_RED);        
       }
     }
     xOffset -= xSz + HIST_X_BAR_SPACE;
  } 
}


void refreshDistance() {
   // No need to refresh everytime
  if ( ui.previous_display != ui.selected_display ) {
    tft.fillRect(HIST_X_OFFSET,HIST_Y_OFFSET-18,HIST_X_TXTSIZE,18,TFT_BLACK);
    tft.fillRect(HIST_X_OFFSET,HIST_Y_OFFSET,HIST_X_SIZE,HIST_Y_SIZE,TFT_BLACK);
    tft.setFreeFont(FF25);    
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Distance",HIST_X_OFFSET,HIST_Y_OFFSET-18,GFXFF);
    tft.drawRoundRect(HIST_X_OFFSET,HIST_Y_OFFSET,HIST_X_SIZE,HIST_Y_SIZE,R_SIZE,TFT_WHITE);
    ui.previous_display = ui.selected_display;
  }

  // clean the bar
  int xSz = (HIST_X_SIZE - (HIST_X_OFFSET+HIST_X_BAR_OFFSET + MAXBUFFER*HIST_X_BAR_SPACE)) / MAXBUFFER;
  int xOffset = HIST_X_OFFSET+HIST_X_SIZE-xSz-HIST_X_BAR_SPACE;
  for ( int i = 0 ; i < MAXBUFFER ; i++ ) {
     tft.fillRect(xOffset,HIST_Y_OFFSET+2,xSz,HIST_Y_SIZE-4,TFT_BLACK);
     xOffset -= xSz + HIST_X_BAR_SPACE;
  }
  // Redraw lines
  int yOffset = HIST_Y_OFFSET+HIST_Y_SIZE-1;
  int lastY = yOffset+21;
  char sTmp[10];
  for ( uint32_t i = 5000 ; i < MAX_DIST ; i+= 5000 ) {
    int y = yOffset-(log((((i/500) * 2 * (i+5000)) / MAX_DIST))*26);
    tft.drawLine(HIST_X_OFFSET+50,y,HIST_X_SIZE-2,y,TFT_GRAY20);
    if ( lastY - y > 20 ) {
      sprintf(sTmp,"%dkm",i/1000); 
      tft.setFreeFont(FS9);    
      tft.setTextColor(TFT_GRAY);
      tft.drawString(sTmp,HIST_X_OFFSET+5,y-15,GFXFF);
      lastY = y;      
    }   
  }
  xOffset = HIST_X_OFFSET+HIST_X_SIZE-xSz-HIST_X_BAR_SPACE;
  for ( int i = 0 ; i < state.elements ; i++ ) {
     int idx = getIndexInBuffer(state.elements-(i+1));
     if ( idx != MAXBUFFER ) {
       if ( state.hs[idx] != NODATA ) {
          uint16_t minDistance = state.minDistance[idx];
          uint16_t maxDistance = state.maxDistance[idx];
          int miny, maxy;
          if ( minDistance >= 5000 ) {
             miny = yOffset-(log((((minDistance/500) * 2 * (minDistance+5000)) / MAX_DIST))*26);
          } else {
             // linear in the area
             miny  = yOffset-(log((((5000/500) * 2 * (5000+5000)) / MAX_DIST))*26);
             miny  = yOffset - ( minDistance * (yOffset-miny) ) / 5000; 
          }
          if ( maxDistance >= 5000 ) {
             maxy = yOffset-(log((((maxDistance/500) * 2 * (maxDistance+5000)) / MAX_DIST))*26);
          } else {
             // linear in the area
             maxy  = yOffset - (log((((5000/500) * 2 * (5000+5000)) / MAX_DIST))*26);
             maxy  = yOffset - ( maxDistance * (yOffset-maxy) ) / 5000 - 2; 
          }
          if ( minDistance == maxDistance ) {
             tft.fillRect(xOffset,maxy,xSz,1,TFT_GREEN); 
          } else {
             tft.fillRect(xOffset,maxy,xSz,(miny-maxy),TFT_GREEN); 
          }         
       } else {
          tft.drawLine(xOffset+1,yOffset+3-5,xOffset+xSz-2,yOffset-3-5,TFT_RED);
          tft.drawLine(xOffset+1,yOffset-3-5,xOffset+xSz-2,yOffset+3-5,TFT_RED);        
       }
     }
     xOffset -= xSz + HIST_X_BAR_SPACE;
  } 
}


/**
 * Refresh the GPS state indicator
 */
void refreshGps() {
  int xOffset = X_OFFSET+4;
  int yOffset = Y_OFFSET+2*Y_SIZE+5;
  if ( gps.isReady ) {
    if (  gpsQualityIsGoodEnough() ) {
       tft.fillRoundRect(xOffset,yOffset,10,10,5,TFT_GREEN);  
    } else {
       tft.fillRoundRect(xOffset,yOffset,10,10,5,TFT_ORANGE);  
    }
  } else {
     if ( gps.rxStuff ) {
       tft.fillRoundRect(xOffset,yOffset,10,10,5,TFT_BLACK);   
       delay(50);   
     }
     tft.fillRoundRect(xOffset,yOffset,10,10,5,TFT_RED);
  }
}

/**
 * Display GPS data (mostly for debugging purpose and curiosity)
 */
#define TXT_TIME_OFF_Y      (HIST_Y_OFFSET+10)
#define TXT_LAT_OFF_Y       (HIST_Y_OFFSET+35)
#define TXT_LNG_OFF_Y       (HIST_Y_OFFSET+60)
#define TXT_ALT_OFF_Y       (HIST_Y_OFFSET+85)
#define TXT_QUA_OFF_Y       (HIST_Y_OFFSET+110)
#define TXT_BAT_OFF_Y       (HIST_Y_OFFSET+135)
#define TXT_ALL_OFF_X       (HIST_X_OFFSET+5)
#define TXT_ALL_VALUE_OFF_X (HIST_X_OFFSET+21+85)
void refreshGpsDetails() {
  if ( ui.previous_display != ui.selected_display ) {
    tft.fillRect(HIST_X_OFFSET,HIST_Y_OFFSET-18,HIST_X_TXTSIZE,18,TFT_BLACK);
    tft.fillRect(HIST_X_OFFSET,HIST_Y_OFFSET,HIST_X_SIZE,HIST_Y_SIZE,TFT_BLACK);
    tft.setFreeFont(FF25);    
    tft.setTextColor(TFT_WHITE);
    tft.drawString("GPS",HIST_X_OFFSET,HIST_Y_OFFSET-18,GFXFF);
    tft.drawRoundRect(HIST_X_OFFSET,HIST_Y_OFFSET,HIST_X_SIZE,HIST_Y_SIZE,R_SIZE,TFT_WHITE);
    ui.previous_display = ui.selected_display;
  }
  // Clear the whole area, all is displayed back
  tft.fillRect(HIST_X_OFFSET+2,HIST_Y_OFFSET+2,HIST_X_SIZE-4,HIST_Y_SIZE-4,TFT_BLACK);

  char sTmp[64];
  tft.setFreeFont(FM9);    
  tft.setTextColor(TFT_GRAY);

  #ifdef DEBUGGPS
  if ( !gps.isReady) {
    sprintf(sTmp,"Uptime %d s", millis()/1000); 
    tft.drawString(sTmp,TXT_ALL_OFF_X,TXT_TIME_OFF_Y,GFXFF);

    sprintf(sTmp,"GPS not ready (fixing)");
    tft.drawString(sTmp,TXT_ALL_OFF_X+28,TXT_LAT_OFF_Y,GFXFF);

    // Display NMEA String
    if (gps.lastNMEA && *gps.lastNMEA) {
      uint8_t nmea_len = strlen(gps.lastNMEA);
      uint8_t y_offset = TXT_LNG_OFF_Y;
      uint8_t index = 0;
      // Display each char of last NMEA String
      for (int i=0; i<=nmea_len; i++) {
        sTmp[index] = gps.lastNMEA[i];
        // end of line or string?
        if (index++ >= 27 || i==nmea_len) {
          sTmp[index] = 0x00; // End of String
          tft.drawString(sTmp,TXT_ALL_OFF_X, y_offset,GFXFF);
          y_offset+=25; // Next display Line
          index=0;      // New Line to display
        } 
      }
    } 
    sprintf(sTmp,"Battery:   %d mV (%d%%)",state.batVoltage, state.batPercent);
    tft.drawString(sTmp,TXT_ALL_OFF_X,TXT_BAT_OFF_Y,GFXFF);
  } else {
  #endif
    if ( !gps.hasbeenReady ) {
      // make sure we reset the values
      gps.hour = 0;
      gps.minute = 0;
      gps.second= 0;
      gps.hdop = 0;
      gps.longitude = 0;
      gps.latitude = 0;
      gps.altitude = 0;
      gps.sats = 0;
    } 
    if ( ! gps.isReady ) {
      tft.setTextColor(TFT_RED);
    }
    
    sprintf(sTmp,"Time:      %02d:%02d:%02d", gps.hour, gps.minute, gps.second); 
    tft.drawString(sTmp,TXT_ALL_OFF_X,TXT_TIME_OFF_Y,GFXFF);

    sprintf(sTmp,"Latitude:  %f", gps.latitude/10000000.0);
    tft.drawString(sTmp,TXT_ALL_OFF_X,TXT_LAT_OFF_Y,GFXFF);

    sprintf(sTmp,"Longitude: %f", gps.longitude/10000000.0);
    tft.drawString(sTmp,TXT_ALL_OFF_X,TXT_LNG_OFF_Y,GFXFF);

    sprintf(sTmp,"Altitude:  %d", gps.altitude);
    tft.drawString(sTmp,TXT_ALL_OFF_X,TXT_ALT_OFF_Y,GFXFF);

    if ( gps.isReady && !gpsQualityIsGoodEnough() ) {
      tft.setTextColor(TFT_ORANGE);
    }
    sprintf(sTmp,"Hdop:      %d.%d Sats: %d", gps.hdop/100,gps.hdop-100*(gps.hdop/100), gps.sats);
    tft.drawString(sTmp,TXT_ALL_OFF_X,TXT_QUA_OFF_Y,GFXFF);

    sprintf(sTmp,"Battery:   %d mV (%d%%)",state.batVoltage, state.batPercent);
    tft.drawString(sTmp,TXT_ALL_OFF_X,TXT_BAT_OFF_Y,GFXFF);
  #ifdef DEBUGGPS
  }
  #endif
}

/** ********************************************************************
 * Configuration screen
 */

// Return true when the configuration is valid
static uint8_t _currentItem = CONF_ITEM_ZONE;
static uint8_t _currentColumn = 0;
bool manageConfigScreen(bool interactive, bool firstRun, bool onlyZone) {
  uint8_t change = 0;
  bool keyGet = false;
  if ( firstRun ) {
    displayConfigScreen(_currentItem,_currentColumn,CONF_ACTION_NONE,true,onlyZone);
  }
  do {
    if (digitalRead(WIO_5S_RIGHT) == LOW) {
       if ( displayConfigScreen(_currentItem,_currentColumn,CONF_ACTION_NEXTCOL,false,onlyZone) ) {
         _currentColumn++;
       }
       keyGet = true;
    } else if (digitalRead(WIO_5S_LEFT) == LOW) {
       if ( displayConfigScreen(_currentItem,_currentColumn,CONF_ACTION_PREVCOL,false,onlyZone) ) {
         _currentColumn--;
       }
       keyGet = true;
    } else if (digitalRead(WIO_5S_UP) == LOW) {
       if ( displayConfigScreen(_currentItem,_currentColumn,CONF_ACTION_PREVITEM,false,onlyZone) ) {
         _currentItem--;
         _currentColumn = 0;
       }
       keyGet = true;
    } else if (digitalRead(WIO_5S_DOWN) == LOW) {
       if ( displayConfigScreen(_currentItem,_currentColumn,CONF_ACTION_NEXTITEM,false,onlyZone) ) {
         _currentItem++;
         _currentColumn = 0;
       }
       keyGet = true;
    } else if (digitalRead(WIO_KEY_A) == LOW) {
      change = 4;
       keyGet = true;
    } else if (digitalRead(WIO_KEY_B) == LOW) {
      change = 2;
       keyGet = true;
    } else if (digitalRead(WIO_KEY_C) == LOW) {
      change = 1;
       keyGet = true;
    } else if (digitalRead(WIO_5S_PRESS) == LOW) {
      // save config once verified
      uint32_t sumOfDevEUI = 0, sumOfAppEUI = 0, sumOfAppKEY = 0;
      for ( int i = 0 ; i < 8 ; i++ ) {
        sumOfDevEUI+= loraConf.deveui[i];
        sumOfAppEUI+= loraConf.appeui[i];
        sumOfAppKEY+= loraConf.appkey[i];
        sumOfAppKEY+= loraConf.appkey[i+8];
      }
      if (    loraConf.zone != ZONE_UNDEFINED 
           && sumOfDevEUI > 0
           && sumOfAppEUI > 0
           && sumOfAppKEY > 0
         ) {
          // make sure values are still valid for the new zone
          tst_setPower(state.cPwr);
          tst_setSf(state.cSf);
          // assuming the conf is valid
          state.cnfBack = false;
          if ( loraConf.zone == ZONE_LATER ) {
            state.hidKey = true;
          }
          storeConfig();
          return true;
      } else {
        return false;
      }
      keyGet = true;
    }
    if ( change > 0 ) {
      switch (_currentItem) {
        case CONF_ITEM_ZONE:
          #if HWTARGET == LORAE5
           loraConf.zone = loraConf.zone + 1;
           if ( loraConf.zone > ZONE_MAX ) loraConf.zone = ZONE_MIN;
          #endif
          break;
        case CONF_ITEM_DEVEUI:
          if ( _currentColumn & 1 ) {
            // low quartet
            loraConf.deveui[_currentColumn/2] = ( (loraConf.deveui[_currentColumn/2]+change) & 0x0F ) | ( loraConf.deveui[_currentColumn/2] & 0xF0);
          } else {
            // high quartet
            loraConf.deveui[_currentColumn/2] = ( (loraConf.deveui[_currentColumn/2]+(16*change)) & 0xF0 ) |  (loraConf.deveui[_currentColumn/2] & 0x0F );
          }          
          break;
        case CONF_ITEM_APPEUI:
          if ( _currentColumn & 1 ) {
            // low quartet
            loraConf.appeui[_currentColumn/2] = ( (loraConf.appeui[_currentColumn/2]+change) & 0x0F ) | ( loraConf.appeui[_currentColumn/2] & 0xF0);
          } else {
            // high quartet
            loraConf.appeui[_currentColumn/2] = ( (loraConf.appeui[_currentColumn/2]+(16*change)) & 0xF0 ) |  (loraConf.appeui[_currentColumn/2] & 0x0F );
          }          
          break;
        case CONF_ITEM_APPKEY:
          if ( _currentColumn & 1 ) {
            // low quartet
            loraConf.appkey[_currentColumn/2] = ( (loraConf.appkey[_currentColumn/2]+change) & 0x0F ) | ( loraConf.appkey[_currentColumn/2] & 0xF0);
          } else {
            // high quartet
            loraConf.appkey[_currentColumn/2] = ( (loraConf.appkey[_currentColumn/2]+(16*change)) & 0xF0 ) |  (loraConf.appkey[_currentColumn/2] & 0x0F );
          }          
          break;
      }
      displayConfigScreen(_currentItem,_currentColumn,CONF_ACTION_NONE,false,onlyZone);
      change = 0;
    }
    if ( keyGet ) delay(200);
  } while ( ! interactive );
  return false;
}

void highlightOneElement(uint8_t selectedItem, uint8_t selectedColumn, bool displayed);
#define TXT_ZONE_OFF_Y      (HIST_Y_OFFSET+10)
#define TXT_DEVEUI_OFF_Y    (HIST_Y_OFFSET+35)
#define TXT_APPEUI_OFF_Y    (HIST_Y_OFFSET+60)
#define TXT_APPKEY_OFF_Y    (HIST_Y_OFFSET+85)
#define TXT_APPKEY_OFF_Y2   (HIST_Y_OFFSET+110)
#define TXT_ALL_OFF_X       (HIST_X_OFFSET+5)
#define TXT_ALL_VALUE_OFF_X (HIST_X_OFFSET+5+85)

// return true when action executed
bool displayConfigScreen(uint8_t selectedItem, uint8_t selectedColumn, uint8_t action, bool refreshAll, bool onlyZone) {
  
  // No need to refresh everytime
  if ( refreshAll ) {
    tft.fillRect(HIST_X_OFFSET,HIST_Y_OFFSET-18,HIST_X_TXTSIZE,18,TFT_BLACK);
    tft.fillRect(HIST_X_OFFSET,HIST_Y_OFFSET,HIST_X_SIZE,HIST_Y_SIZE,TFT_BLACK);
    tft.setFreeFont(FF25);    
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Configuration",HIST_X_OFFSET,HIST_Y_OFFSET-18,GFXFF);
    tft.drawRoundRect(HIST_X_OFFSET,HIST_Y_OFFSET,HIST_X_SIZE,HIST_Y_SIZE,R_SIZE,TFT_WHITE);
  } 
  tft.fillRect(TXT_ALL_VALUE_OFF_X,TXT_ZONE_OFF_Y-2,200,120,TFT_BLACK);
  highlightOneElement(selectedItem,selectedColumn,true);

  // Print configuration
  char sTmp[128];
  tft.setFreeFont(FM9);    
  tft.setTextColor(TFT_GRAY);

  char sZone[10];
  switch (loraConf.zone) {
    default:
    case ZONE_UNDEFINED:
        sprintf(sZone,"NA");
        break;
    case ZONE_EU868:
        sprintf(sZone,"EU868");
        break;
    case ZONE_US915:
        sprintf(sZone,"US915");
        break;
    case ZONE_AS923_1:
        sprintf(sZone,"AS923_1");
        break;
    case ZONE_AS923_2:
        sprintf(sZone,"AS923_2");
        break;
    case ZONE_AS923_3:
        sprintf(sZone,"AS923_3");
        break;
    case ZONE_AS923_4:
        sprintf(sZone,"AS923_4");
        break;
    case ZONE_KR920:    
        sprintf(sZone,"KR920");
        break;
    case ZONE_IN865:    
        sprintf(sZone,"IN865");
        break;
    case ZONE_AU915:
        sprintf(sZone,"AU915");
        break;
    case ZONE_LATER:
        sprintf(sZone,"NA");
        break;
  }
  sprintf(sTmp,"Zone:   %s", sZone); 
  tft.drawString(sTmp,TXT_ALL_OFF_X,TXT_ZONE_OFF_Y,GFXFF);

  if ( !onlyZone ) {
    sprintf(sTmp,"DevEUI: %02X%02X%02X%02X%02X%02X%02X%02X",
      loraConf.deveui[0],loraConf.deveui[1],
      loraConf.deveui[2],loraConf.deveui[3],
      loraConf.deveui[4],loraConf.deveui[5],
      loraConf.deveui[6],loraConf.deveui[7]
    ); 
    tft.drawString(sTmp,TXT_ALL_OFF_X,TXT_DEVEUI_OFF_Y,GFXFF);
    
    sprintf(sTmp,"AppEUI: %02X%02X%02X%02X%02X%02X%02X%02X",
      loraConf.appeui[0],loraConf.appeui[1],
      loraConf.appeui[2],loraConf.appeui[3],
      loraConf.appeui[4],loraConf.appeui[5],
      loraConf.appeui[6],loraConf.appeui[7]
    ); 
    tft.drawString(sTmp,TXT_ALL_OFF_X,TXT_APPEUI_OFF_Y,GFXFF);

    if ( !state.hidKey ) {
      sprintf(sTmp,"AppKEY: %02X%02X%02X%02X%02X%02X%02X%02X",
        loraConf.appkey[0],loraConf.appkey[1],
        loraConf.appkey[2],loraConf.appkey[3],
        loraConf.appkey[4],loraConf.appkey[5],
        loraConf.appkey[6],loraConf.appkey[7]
      ); 
      tft.drawString(sTmp,TXT_ALL_OFF_X,TXT_APPKEY_OFF_Y,GFXFF);
    
      sprintf(sTmp,"        %02X%02X%02X%02X%02X%02X%02X%02X",
        loraConf.appkey[8],loraConf.appkey[9],
        loraConf.appkey[10],loraConf.appkey[11],
        loraConf.appkey[12],loraConf.appkey[13],
        loraConf.appkey[14],loraConf.appkey[15]
      ); 
      tft.drawString(sTmp,TXT_ALL_OFF_X,TXT_APPKEY_OFF_Y2,GFXFF);  
    } else {
      sprintf(sTmp,"AppKEY: XXXXXXXXXXXXXXXX"); 
      tft.drawString(sTmp,TXT_ALL_OFF_X,TXT_APPKEY_OFF_Y,GFXFF);
    
      sprintf(sTmp,"        XXXXXXXXXXXXXXXX"); 
      tft.drawString(sTmp,TXT_ALL_OFF_X,TXT_APPKEY_OFF_Y2,GFXFF);  
    }
  }
  // Previous Item & column
  uint8_t prevCol = selectedColumn;
  uint8_t prevItem = selectedItem;
  
  // Clean previous
  switch ( action ) {
    case CONF_ACTION_NEXTITEM:
          if ( onlyZone ) return false;
          if ( selectedItem < CONF_ITEM_LAST) {
            selectedItem++;
            selectedColumn=0;
          }
          else return false;
          break;
    case CONF_ACTION_PREVITEM:
          if ( onlyZone ) return false;
          if ( selectedItem > CONF_ITEM_FIRST) {
            selectedItem--;
            selectedColumn=0;
          }
          else return false;
          break;
    case CONF_ACTION_NEXTCOL:
          switch ( selectedItem ) {
            case CONF_ITEM_ZONE: return false;
            case CONF_ITEM_DEVEUI: 
            case CONF_ITEM_APPEUI:
              if ( selectedColumn < 15 ) {
                selectedColumn++;
              } else return false;
              break;
            case CONF_ITEM_APPKEY:
              if ( selectedColumn < 31 ){
                selectedColumn++;
              } else return false;
              break;
          }
          break;
    case CONF_ACTION_PREVCOL:
          switch ( selectedItem ) {
            case CONF_ITEM_ZONE: return false;
            case CONF_ITEM_DEVEUI: 
            case CONF_ITEM_APPEUI:
            case CONF_ITEM_APPKEY:
              if ( selectedColumn > 0 ) selectedColumn--;
              else return false;
              break;
          }
          break;
  }

  // update drawing
  highlightOneElement(prevItem,prevCol,false);
  highlightOneElement(selectedItem,selectedColumn,true);
  return true;
  
}


// draw a box around 1 field selected by Item and Column
// the max numbers are not computed here
// displayed indicates if we draw or remove the box
void highlightOneElement(uint8_t selectedItem, uint8_t selectedColumn, bool displayed) {

  uint16_t x,y;
  int color = (displayed)?TFT_GREEN:TFT_BLACK;
  
  switch ( selectedItem ) {
    case CONF_ITEM_ZONE:
      x = TXT_ALL_VALUE_OFF_X;
      y = TXT_ZONE_OFF_Y-2;
      tft.drawRoundRect(x,y,80,18,4,color);
      break;
    case CONF_ITEM_DEVEUI:
      x = 3+TXT_ALL_VALUE_OFF_X+selectedColumn*11;
      y = TXT_DEVEUI_OFF_Y-2;
      tft.drawRoundRect(x,y,12,18,4,color);
      break;
    case CONF_ITEM_APPEUI:
      x = 3+TXT_ALL_VALUE_OFF_X+selectedColumn*11;
      y = TXT_APPEUI_OFF_Y-2;
      tft.drawRoundRect(x,y,12,18,4,color);
      break;
    case CONF_ITEM_APPKEY:
      if ( selectedColumn >= 16 ) {
         y = TXT_APPKEY_OFF_Y2-2;
         selectedColumn -= 16;
      } else {
         y = TXT_APPKEY_OFF_Y-2;
      }
      x = 3+TXT_ALL_VALUE_OFF_X+selectedColumn*11;
      tft.drawRoundRect(x,y,12,18,4,color);
      break;
  }
  
}

// Missing LoRA Board
void LoRaMissing() {
        
      tft.fillRect(0,120-20,320,40,TFT_RED);
      tft.setTextColor(TFT_WHITE);
      tft.setFreeFont(FS9);     // Select the orginal small TomThumb font
      tft.drawString("LoRa board is missing",75,112, GFXFF);  
}
