# Wio Terminal LoRaWAN Field Tester

<img src="img/splash.jpg" alt="LoRaWan tester" width="250"/>

## What's is it about

When deploying a LoRaWAN network you want to know your gateway coverage and measure the radio performance in different places. When deploying Helium network you particularly want to know how many Hotspot around can be touched for a given position.

There are some existing tools for such application but the cost is about $200. The idea of this project is to propose a simple tool you can build on your own with low cost hardware to fulfill this purpose.

The solution is based on low cost products, easy to find for about $50:
- [Wio Terminal](https://wiki.seeedstudio.com/Wio-Terminal-Getting-Started/)
- [RFM95](https://www.disk91.com/2019/technology/lora/hoperf-rfm95-and-arduino-a-low-cost-lorawan-solution/)
- Quectel L80 / L86 GNSS

### Current status
- Ready, documentation in progress.

### Features
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
- Screen shot and user guide
- RFM95 Schematics
- GPS position reporting with Helium mapper integration
- LiPo charging mode detection
- PCB availbale
- Enclosure available
- Serial port LoRaWan setup configuration
- Setup with Cargo (helium tracking platform)

### Comming soon features
- sdcard data storage

## Get your own device

The WioLoRaWANFieldTester board can be
- [Make](board/README.md)
- [Buy](https://shop.ingeniousthings.fr/products/helium-lorawan-field-tester-and-mapper-kit) as a kit

## Quick installation and setup

Read and follow the steps in [WioLoRaWANFieldTester configuration documentation](doc/SETUP.md)

## Use it

Read the [User Guide documentation](doc/UserGuide.md)

## Contribute to developement

Read the [developer documentation](doc/DEVELOPMENT.md)








