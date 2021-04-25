# Wio Terminal LoRaWAN Field Tester

<img src="img/splash.jpg" alt="LoRaWan tester" width="250"/>

## What's is it about

When deploying a LoRaWAN network you want to know your gateway coverage and measure the radio performance in different places. When deploying Helium network you particularly want to know how many Hotspot around can be touched for a given position.

There are some existing tools for such application but the cost is about $200. The idea of this project is to propose a simple tool you can build on your own with low cost hardware to fulfill this purpose.

The solution is based on low cost products, easy to find for about $50:
- [Wio Terminal](https://wiki.seeedstudio.com/Wio-Terminal-Getting-Started/)
- [RFM95](https://www.disk91.com/2019/technology/lora/hoperf-rfm95-and-arduino-a-low-cost-lorawan-solution/)

## Current status
- Working, documentation in progress to reproduce it on your own.

## Features
- Easy selection for Power / Sf / max retries
- Duty cycle status display
- Last setting flash memory backup
- Graph selection display
	- Rx Rssi - ack Rssi level 
	- Rx Snr - ack Snr level
	- Retries - number of retry before getting a ack
- Manual / Automatic mode switch
- Display last frame information
- Network side RSSI / SNR / #of station
- Your favorite network splash screen (select in config.h)

## Comming soon feature
- Screen shot and user guide
- RFM95 Schematics
- PCB
- Serial port LoRaWan setup configuration
- GPS extension
- sdcard data storage

## How it works

The LoRaWan Field tester is basically sending a frame on demand or in regular basis and wait for an ACK. We can obtain the ACK RX power, eventually the number of retries needed to get it. Then this message is passed to a backend service. This service is responding in a downlink with the network reception level (TX rssi) and the number of hotspots involved in the reception.

+----+-----+
+ <img src="img/mainScreen.jpg" alt="LoRaWan tester" width="250"/> + <img src="img/Mainscreen-2.jpg" alt="LoRaWan tester" width="250"/> +
+----+-----+



## Installation

Details comming soon.