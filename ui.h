#include "testeur.h"

#ifndef __UI_H
#define __UI_H

#define MODE_MANUAL       0     // manual message (default)
#define MODE_AUTO_5MIN    1     // every 5 minutes (consider DC)
#define MODE_AUTO_1MIN    2     // every 1 minutes (consider DC)
#define MODE_MAX_RATE     3     // every time is is possible after DC

#define MODE_MAX          3

typedef struct s_ui {
  uint8_t selected_menu;    // What menu zone is selected for settings
  uint8_t selected_mode;    // What mode do we want ? manual / auto
  e_state displayed_state;  // showning LoRaWan join state
  uint8_t transmit_v;       // Can transmit due to dity cycle - count down ?

  uint8_t lastWrId;
  uint8_t selected_display; // What graph we want to display
  uint8_t previous_display;
  bool    hasClick;         // Hit on button
  
} ui_t;

extern ui_t ui;

void initScreen();
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
#endif
