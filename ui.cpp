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
#include <Arduino.h>
#include "config.h"
#include "fonts.h"
#include "testeur.h"
#include "TFT_eSPI.h"
#include "ui.h"
#include "LoRaCom.h"

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

#define MAX_SNR 40
#define MAX_RETRY 8
#define MAX_HS 20

#define SELECTED_NONE   0
#define SELECTED_POWER  1
#define SELECTED_SF     2
#define SELECTED_RETRY  3

#define DISPLAY_UNKNONW    0
#define DISPLAY_RSSI_HIST  1
#define DISPLAY_SNR_HIST   2
#define DISPLAY_RETRY_HIST 3
#define DISPLAY_TXRSSI     4
#define DISPLAY_TXHS       5

ui_t ui;

void initScreen() {
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);

  // Totally unusefull so totally mandatory
  #ifdef WITH_SPLASH 
    tft.drawRoundRect((320-200)/2,200,200,10,5,TFT_WHITE);
    for ( int i = 10 ; i < 100 ; i+=4 ) {
      tft.fillRoundRect((320-200)/2+2,202,((204*i)/100),6,3,TFT_WHITE);
      #ifdef WITH_SPLASH_HELIUM
        draw_splash_helium(HELIUM_XCENTER, (240-100)/2, i);
      #endif
      #ifdef WITH_SPLASH_TTN  
        draw_splash_ttn(TTN_XCENTER, (240-85)/2, i);
      #endif
  }
  delay(1500);
#endif

  tft.fillScreen(TFT_BLACK);

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
  
  // draw mask
  refreshPower(); 
  refreshSf();
  refreshRetry();
  refreshState();
  refreshMode();
  refreshLastFrame();

  pinMode(WIO_KEY_A, INPUT_PULLUP);
  pinMode(WIO_KEY_B, INPUT_PULLUP);
  pinMode(WIO_KEY_C, INPUT_PULLUP);
  pinMode(WIO_5S_UP, INPUT_PULLUP);
  pinMode(WIO_5S_DOWN, INPUT_PULLUP);
  pinMode(WIO_5S_LEFT, INPUT_PULLUP);
  pinMode(WIO_5S_RIGHT, INPUT_PULLUP);
  pinMode(WIO_5S_PRESS, INPUT_PULLUP); 
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
      default:
         break;   
    }  
    hasAction=true;
  } else if (digitalRead(WIO_5S_LEFT) == LOW) {
    configHasChanged = true;
    switch ( ui.selected_display ) {
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
     if ( state.retry[idx] != LOSTFRAME ) {
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
        tft.drawLine(xOffset+1,HIST_Y_OFFSET+3,xOffset+xSz-2,HIST_Y_OFFSET-3,TFT_RED);
        tft.drawLine(xOffset+1,HIST_Y_OFFSET-3,xOffset+xSz-2,HIST_Y_OFFSET+3,TFT_RED);        
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
  int yStep10 = ((HIST_Y_OFFSET+HIST_Y_SIZE-10) - ( HIST_Y_OFFSET - 5 )) / (MAX_SNR/10);  // step for 10 SNR
  tft.drawLine(HIST_X_OFFSET+2,yOffset,HIST_X_SIZE-2,yOffset,TFT_GRAY);  
  for ( int i = 10 ; i < MAX_SNR ; i+= 10 ) {
    int y = yOffset-(yStep10*i)/10;
    if ( i % 10 == 0 ) {
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
     if ( idx != MAXBUFFER ) {
        int snr = state.snr[idx];
        uint16_t color = TFT_GREEN;
        if ( snr < 5 ) color = TFT_RED;
        else if (snr < 10 ) color = TFT_ORANGE;
        else if (snr < 20 ) color = TFT_DARKGREEN;
        tft.fillRect(xOffset,yOffset-(snr*yStep10)/10,xSz,(snr*yStep10)/10,color);
     } else {
        tft.drawLine(xOffset+1,yOffset+3,xOffset+xSz-2,yOffset-3,TFT_RED);
        tft.drawLine(xOffset+1,yOffset-3,xOffset+xSz-2,yOffset+3,TFT_RED);        
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
     if ( idx != MAXBUFFER && state.hs[idx] != NODATA ) {
       if ( idx != MAXBUFFER ) {
          int minRssi = state.worstRssi[idx];
          int maxRssi = state.bestRssi[idx];
          uint16_t color = TFT_GREEN;
          if ( maxRssi > 8 ) maxRssi = 8; // avoid drawing over the graph.
          if ( minRssi > 8 ) minRssi = 8;
          
          if ( maxRssi < -125 ) color = TFT_RED;
          else if (maxRssi < -100 ) color = TFT_ORANGE;
          else if (maxRssi < -80 ) color = TFT_DARKGREEN;
  
          if ( minRssi != maxRssi ) {
            tft.fillRect(xOffset,(HIST_Y_OFFSET)+(-minRssi+10),xSz,(maxRssi-minRssi),color);
          } else {
            tft.fillRect(xOffset,(HIST_Y_OFFSET)+(-minRssi+10)-1,xSz,2,color);
          }
       } else {
          tft.drawLine(xOffset+1,HIST_Y_OFFSET+3,xOffset+xSz-2,HIST_Y_OFFSET-3,TFT_RED);
          tft.drawLine(xOffset+1,HIST_Y_OFFSET-3,xOffset+xSz-2,HIST_Y_OFFSET+3,TFT_RED);        
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
     if ( idx != MAXBUFFER && state.hs[idx] != NODATA ) {
       if ( idx != MAXBUFFER && state.retry[idx] != LOSTFRAME ) {
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
