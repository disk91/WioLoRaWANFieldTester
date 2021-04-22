#ifndef __LORACOM_H__
#define __LORACOM_H__

#include "lmic.h"

void loraSetup(void);
void do_send(uint8_t port, uint8_t * data, uint8_t sz, _dr_configured_t dr, uint8_t pwr, bool acked, uint8_t retries );
void loraLoop(void);
boolean canLoraSleep(void);
boolean canLoRaSend();
uint32_t nextPossibleSendMs();

#endif
