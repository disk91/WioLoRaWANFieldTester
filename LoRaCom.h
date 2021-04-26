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
