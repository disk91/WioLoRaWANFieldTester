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
#include "TFT_eSPI.h"
#include "testeur.h"

#ifndef __UI_H
#define __UI_H

#define MODE_MANUAL       0     // manual message (default)
#define MODE_AUTO_1H      1     // every  1 hours   (consider DC)
#define MODE_AUTO_15MIN   2     // every 15 minutes (consider DC)
#define MODE_AUTO_5MIN    3     // every  5 minutes (consider DC)
#define MODE_AUTO_1MIN    4     // every  1 minutes (consider DC)
#define MODE_MAX_RATE     5     // every time is is possible after DC

#define MODE_MAX          5

#define DISPLAY_UNKNONW    0
#define DISPLAY_RSSI_HIST  1
#define DISPLAY_SNR_HIST   2
#define DISPLAY_RETRY_HIST 3
#define DISPLAY_TXRSSI     4
#define DISPLAY_TXHS       5
#define DISPLAY_DISTANCE   6
#define DISPLAY_GPS        7


#ifdef WITH_SPLASH_HELIUM
  #ifdef WITH_SPLASH_TTN
    #define HELIUM_XCENTER (160-100)/2
    #define TTN_XCENTER 160+(160-100)/2
  #else
    #define HELIUM_XCENTER (320-100)/2
  #endif
#else
  #define TTN_XCENTER (320-100)/2
#endif

typedef struct s_ui {
  uint8_t selected_menu;    // What menu zone is selected for settings
  uint8_t selected_mode;    // What mode do we want ? manual / auto
  e_state displayed_state;  // showing LoRaWan join state
  uint8_t transmit_v;       // Can transmit due to duty cycle - count down ?

  uint8_t lastWrId;
  uint8_t selected_display; // What graph we want to display
  uint8_t previous_display;
  bool    hasClick;         // Hit on button
  bool    alertMode;        // LiPo alert is on
  uint32_t lastGpsUpdateTime; // Last GPS display update time
} ui_t;

extern ui_t ui;
extern TFT_eSPI tft;

void configPending();
void initScreen();
void displaySplash();
void displayTitle();
void clearScreen();
void screenSetup();
void refresUI();

void refreshPower(); 
void refreshSf();
void refreshRetry();
void refreshState();
void refreshRssiHist();
void refreshSnrHist();
void refreshRetryHist();
void refreshMode();
void refreshLastFrame();
void refreshTxRssi();
void refreshTxHs();
void refreshGpsDetails();
void refreshGps();
bool refreshLiPo();
void refreshDistance();
void draw_splash_helium(int xOffset, int yOffset, int density);
void draw_splash_ttn(int xOffset, int yOffset, int density);
bool manageConfigScreen(bool interactive, bool firstRun, bool onlyZone);
bool displayConfigScreen(uint8_t selectedItem, uint8_t selectedColumn, uint8_t action, bool refreshAll, bool onlyZone);
void LoRaMissing();

#define CONF_ACTION_NONE     0
#define CONF_ACTION_MODIFY   1
#define CONF_ACTION_NEXTITEM 2
#define CONF_ACTION_PREVITEM 3
#define CONF_ACTION_NEXTCOL  4
#define CONF_ACTION_PREVCOL  5
#define CONF_ACTION_UP1      6
#define CONF_ACTION_UP2      7
#define CONF_ACTION_UP4      8

#define CONF_ITEM_ZONE       1 
#define CONF_ITEM_DEVEUI     2 
#define CONF_ITEM_APPEUI     3 
#define CONF_ITEM_APPKEY     4 
#define CONF_ITEM_LAST       CONF_ITEM_APPKEY
#define CONF_ITEM_FIRST      CONF_ITEM_ZONE
#endif
