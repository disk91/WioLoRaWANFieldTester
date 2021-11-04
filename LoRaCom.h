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
#ifndef __LORACOM_H__
#define __LORACOM_H__

#define MAX_RETRY 5 

typedef struct {
  uint8_t deveui[8];
  uint8_t appeui[8];
  uint8_t appkey[16];
  uint8_t zone;
} loraConf_t;

extern loraConf_t loraConf;

void loraSetup(void);
void do_send(uint8_t port, uint8_t * data, uint8_t sz, uint8_t _dr, uint8_t pwr, bool acked, uint8_t retries );
void loraLoop(void);
boolean canLoraSleep(void);
boolean canLoRaSend();
uint32_t nextPossibleSendMs();
bool processLoRaConfig(void);

#define ZONE_UNDEFINED  0
#define ZONE_EU868      1
#define ZONE_US915      2
#define ZONE_AS923_1    3
#define ZONE_AS923_2    4
#define ZONE_AS923_3    5
#define ZONE_AS923_4    6
#define ZONE_KR920      7
#define ZONE_IN865      8
#define ZONE_AU915      9
#define ZONE_MAX        ZONE_AU915
#define ZONE_MIN        ZONE_EU868

#endif
