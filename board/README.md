# This folders contains the information to make your own board
- lorawan-tester.pdf is the schematics file
- lorawan-tester-20210429.zip is the gerber archive you can use asis with most of the PCB manufacturers

# Board versions

You can use this board in different versions:
* The LoRaWan only version, without GPS and powered by a USB-C battery connected on the WioTerminal. This version is really easy to realize as the number of component to solder is limited.
* The LoRaWan + GPS version, powered by the USB-C battery connected to the WioTerminal.
* The Full version, with GPS and LiPo charging circuit allows to access the full features including the Mapper information reporting. This version includes some CMS a bit complicated to solder.

A single PCB can support the three versions, it just depends on the components you decide to place on it.
You can order the board directly to different service on Internet:
- European PCB maker where the board can be accessed : [Aisler](https://aisler.net/p/MXYVQSVT)
- North American PCB maker the board can be accessed : [OshPark](https://oshpark.com/shared_projects/1TNqhC7U)


**comming soon: kit with all the components to be assembled. I'll update with link**

# BOM

## LoRaWan only version
|Ref| Description |
|---|-----------------------------------------------------------|
|J1 | 40 pins (2x20) 2.54 header CMS |
|U2 | RFM95 (version 868MHz or 915MHz according to your zone) |
|R6 | 0 ohm 0603 resistor (a wire can be used) |
|U3 | Edge PCB SMA connector |

Estimated BOM price around $10

## LoRaWan + GPS version 
|Ref| Description |
|---|-----------------------------------------------------------|
|J1 | 40 pins (2x20) 2.54 header CMS (samtec TSM-120-02-L-DV) |
|U2 | RFM95 (version 868MHz or 915MHz according to your zone) |
|R6 | 0 ohm 0603 resistor (a wire can be used) |
|U3 | Edge PCB SMA connector |
|M1 | Quectel L86 (L80 should be ok) |
|C3 | Capacitor 0.1uF 0603 CMS |
|C4 | Capacitor 4.7uF 0805 CMS |
|C1 | Capacitor 4.7uF 0805 CMS |

Estimated BOM price around $20 

## Full version 
|Ref| Description |
|---|-----------------------------------------------------------|
|J1 | 40 pins (2x20) 2.54 header CMS (samtec TSM-120-02-L-DV) |
|U2 | RFM95 (version 868MHz or 915MHz according to your zone) |
|R6 | 0 ohm 0603 resistor (a wire can be used) |
|U3 | Edge PCB SMA connector |
|M1 | Quectel L86 (L80 should be ok) |
|C3 | Capacitor 0.1uF 0603 CMS |
|C4 | Capacitor 4.7uF 0805 CMS |
|C1 | Capacitor 4.7uF 0805 CMS |
|D1 | LED CMS 0805 |
|U1 | MPC73831 (LiPo Charging Circuit) |
|R2 | 2 Kohm 0603 resistor |
|R4 | 1 Mohm 0603 resistor |
|R3 | 1 Mohm 0603 resistor |
|R1 | 1 Kohm 0603 resistor |
|J2 | JST 2mm LiPo Connector S2B-PH-K-S(LF)(SN)|
|S1 | Switch C&K - OS102011MA1QN1
|   | LiPo Battery 3.7V 500mAh - 1600 mAh with JST connector

Estimated BOM price around $25 

When connecting a LiPo battery, please make sure of the battery polarity according to the polarity printed on the PCB. There is no rules about LiPo battery polarity.

