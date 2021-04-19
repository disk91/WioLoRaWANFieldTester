#include "testeur.h"

#ifndef __UI_H
#define __UI_H

typedef struct s_ui {
  uint8_t selected_menu;    // What menu zone is selected for settings
  e_state displayed_state;  // showning LoRaWan join state
  uint8_t transmit_v;       // Can transmit due to dity cycle - count down ?

  uint8_t lastWrId;
  uint8_t selected_display; // What graph we want to display
  uint8_t previous_display;
  
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
#endif
