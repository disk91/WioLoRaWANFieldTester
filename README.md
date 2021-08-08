# Wio Terminal LoRaWAN Field Tester

<img src="img/splash.jpg" alt="LoRaWan tester" width="250"/>

## What's is it about

When deploying a LoRaWAN network you want to know your gateway coverage and measure the radio performance in different places. When deploying Helium network you particularly want to know how many Hotspot around can be touched for a given position.

There are some existing tools for such application but the cost is about $200. The idea of this project is to propose a simple tool you can build on your own with low cost hardware to fulfill this purpose.

The solution is based on low cost products, easy to find for about $50:
- [Wio Terminal](https://wiki.seeedstudio.com/Wio-Terminal-Getting-Started/)
- [RFM95](https://www.disk91.com/2019/technology/lora/hoperf-rfm95-and-arduino-a-low-cost-lorawan-solution/)

You can directly purchase components Kit with PCB on [ingeniousthings shop](https://shop.ingeniousthings.fr/products/helium-lorawan-field-tester-and-mapper-kit)

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
- Screen shot and user guide
- RFM95 Schematics
- GPS position reporting with Helium mapper integration
- LiPo charging mode detection
- PCB availbale
- Enclosure available
- Serial port LoRaWan setup configuration
- Setup with Cargo (helium tracking platform)

## Comming soon features
- sdcard data storage

## Quick installation and setup

### Install Firmware

- You can directly upload the following binaries:
  * [EU868 with GPS enable](binaries/WioLoRaWANFieldTester_EU868_GPS.uf2)
  * [EU868 with GPS disable](binaries/WioLoRaWANFieldTester_EU868_NOGPS.uf2)
  * [US915 with GPS enable](binaries/WioLoRaWANFieldTester_US915_GPS.uf2)
  * [US915 with GPS disable](binaries/WioLoRaWANFieldTester_US915_NOGPS.uf2)
- Once downloaded, you load the firwmare just by drag & drop it to the mounted drive when connecting the WioTerminal to your computer
  * switch the device in programming mode switching reset button ON / RESET / ON / RESET / ON
  * An "Arduino" disk appear on your computer
  * Drag and drop the selected firmware into that drive.
  * Fimware is flashing, device is rebooting, you should see "SETUP CREDENTIALS" on the screen

### Setup LoRaWan Credentials
  * Open a Serial console (putty or any other serial tool) with speed 9600.
  * Get credentials from console.helium.com (or any other LoRaWan network server)
    * deveui
    * appeui
    * appkey
  * Set the parameters in the Serial console following:
  You need to enter 1 line after the other, replace the differents IDs by the values obtained from the helium console. 
  ```
  D=DB6B416730000000
  A=7262F3850E000000
  K=D2B1F00A451E34931900000000000000
  ```
  * You should see on the console
  ```
  DEVEUI:DB6B416730000000
  OK
  APPEUI:7262F3850E000000
  OK
  APPKEY:D2B1F00A451E34931900000000000000
  OK
  LoRaWan configuration OK
  ```
  * The device will reboot with the new configuration and is ready.

### Setup Helium Console integration with backend
See [related blogpost](https://www.disk91.com/2021/technology/lora/low-cost-lorawan-field-tester/) to get all the details. This integration is supporting the loopback feature and the mapper connectivity feature.


## How it works

The LoRaWan Field tester is basically sending a frame on demand or in regular basis and wait for an ACK. We can obtain the ACK RX power, eventually the number of retries needed to get it. Then this message is passed to a backend service. This service is responding in a downlink with the network reception level (TX rssi) and the number of hotspots involved in the reception.

<img src="img/mainScreen.jpg" alt="LoRaWan tester" width="250"/> <img src="img/Mainscreen-2.jpg" alt="LoRaWan tester" width="250"/>

You can select the transmission power to be used, the spread-factor (speed), the maximum retries allowed by selecting the parameter to modify with the button located on the top side. Once selected, you change the values with UP & DOWN from the 5 directions button.

When none of these parameters are selected, you can change the mode with the UP & DOWN. The following modes are available:
- Manual : a frame is fired when pushing the 5 ways button. The downlink response is obtained by pooling.
- Auto 5m : a frame is fired automatically every 5 minutes. The downlink response is obtained by pooling.
- Auto 1m : a frame is fired automatically every 1 minute. The downlink response is obtained by pooling.
- Max Rate : a frame is fired as soon as the device can regarding the eventual Duty Cycle. Downlink response will be received later on the flow and will onlu be monitored from the historical graph.

The status is displayed on the screen and can be:
- Disc - disconnected or not yet connected
- Join - device is joining the network
- Cnx - device has joined, ready to fire messages
- Tx - transmisison in progress (orange when doing a retry)
- Dwn - communication in progress to retrieve the downlink containing the network side informations

The Green / Red square on the righ is indicating the duty-cycle status when applicable. Green is ready to communicate, Red is duty-cycle with the count-down before being ready.

The Red / Orange / Green circle on the left is indicating the GPS status (when activated). Red is not yet ready, Orange is position aquired but quality is poor to be reported. Green is good quality position ; it will be reported then.

The Green / Orange / Red bar on the left is indicating the battery level status (when activated)

The last communication result is displayed on the 2 lines under the settings.
- The first line shows the device side information ( from left to right ):
	* The sequence ID of the frame
	* The Rssi of the Ack message as received by the Field tester
	* The Snr of the Ack message as received by the Field tester
	* The number of repeat before obtaining the ack response
- The second line shows the network side information ( from left to right):
	* The minimum RSSI value from the different hotspots having received the frame
	* The maximum RSSI value from the different hotspots having received the frame
	* The number of hotspot having received the frame.

Rq : due to the way Helium works or due to the TTNv2 to TTNv3 migration, the second line could display a reduced number of Hotspot compared to the reality. For Helium where this information could be critical, make sure you have done all the configuration steps and bought all the frame setting. Currently Helium has a bug and not execute this action correctly so in most of the cases you will have only one Hotspot response. I'll update that documentation once it will be fixed.

All these information can be displays with an historical graph you select using the LEFT & RIGHT buttons:
<img src="img/RX RSSI.jpg" alt="Ack Rssi history" width="250"/><img src="img/RXSNR.jpg" alt="Ack Snr history" width="250"/>

<img src="img/RETRY.jpg" alt="Uplink Retry history" width="250"/><img src="img/TXRSSSI.jpg" alt="Network side Rssi history" width="250"/><img src="img/Hotspots.jpg" alt="Hotspots involved history" width="250"/>

In the historical graph, a red cross is indicating a packet loss a green cross a 0 value.
The TX Rssi graph is displaying a min-max bar, this is why you see just a line for a single hotspot response.

## Schematics

Take a look at *board* directory for details on PCB & components.

Here is the simplified version of the schematics for DiY implementation.
<img src="img/Wio-LoRaWan-FieldTester-schema.png" alt="basic schematic" width="500"/> 

## Required libraries
* GPS - Adafruit GPS library version 1.5.4
* File system - Seeed Arduino FS version 2.0.3
* File system - Seeed Ardunino SFUD version 2.0.1
* LoRaWAn - Mcci...
* Flash - FlashStorage by various version 1.0.0

## Enclosure

3D printed enclosure can be found in the enclosure directory. You will find the two parts of the enclosure in different STL files. You will also find the FreeCad source file in case you want to modify /improve it.

The enclosure is closed and attached to the WioTerminal with [2 screws DIN965, M2 L16mm](https://www.bricovis.fr/produit-vis-a-tete-fraisee-pozidriv-acier-zingue-blanc-din-965-tfzzn/#TFZ02/016ZN)


## Installation

The installation details are available in the related [Wio LoRaWan Field tester on disk91.com](https://www.disk91.com/?p=5187) 

## Integration with backend

### Use disk91 backend
See [related blogpost](https://www.disk91.com/2021/technology/lora/low-cost-lorawan-field-tester/) to get all the details. This integration is supporting the loopback feature and the mapper connectivity feature.

This allow to automatically get your positions reported to Helium mapper and also allows to obtain network statistics and get them displayed on the WioTerminal. 


### Standalone helium mapper integration (if you do not want to use disk91 backend - you will lost part of the statistics)

**Do not use disk91 backend + standalone mapper integration it is redundand** 

The following Frame format are used:
**uplink format on port 1:**
| Byte          | Usage                          |
|---------------|--------------------------------|
| `0 - 5`       | GSP position see [here](https://www.disk91.com/2015/technology/sigfox/telecom-design-sdk-decode-gps-frame/) for details. Decoding see below |
| `6 - 7`       | Altitude in meters + 1000m ( 1100 = 100m ) |
| `8`           | HDOP * 10 (11 = 1.1) |
| `9`           | Sats in view |

When the GPS position is invalid of GPS is disable, the frame is full of 0

**donwlink response format on port 2:**
| Byte          | Usage                          |
|---------------|--------------------------------|
| `0`           | Sequence ID % 255              |
| `1`           | Min Rssi + 200 (160 = -40dBm)  |
| `2`           | Max Rssi + 200 (160 = -40dBm)  |
| `3`           | Min SNR + 100 (80 = -20dBm)    |
| `4`           | Max SNR + 100 (80 = -20dBm)    |
| `5`           | Seen hotspot                   |


The following integration and payload transformation allows to decode the gps position and report is to mapper. Thank you Seb for the contribution.

Create a _Functions_ type _Decoder_ / _Custom Script_ and attach it to a mapper integration callback as it is described in this [helium mapper integration page](https://docs.helium.com/use-the-network/coverage-mapping/mappers-quickstart/)

```
/*
  Helium console function for LoRaWan Field Tester sending mapping information to mappers backend.

  https://www.disk91.com/2021/technology/lora/low-cost-lorawan-field-tester/

  Result is visible on https://mappers.helium.com/

  Built from information available on:
    https://docs.helium.com/use-the-network/coverage-mapping/mappers-api/
    https://github.com/disk91/WioLoRaWANFieldTester/blob/master/WioLoRaWanFieldTester.ino
    https://www.disk91.com/2015/technology/sigfox/telecom-design-sdk-decode-gps-frame/

  Integartion:
    POST https://mappers.helium.com/api/v1/ingest/uplink
*/

function Decoder(bytes, port) { 
  var payload = {};
  
  var lonSign = (bytes[0]>>7) & 0x01 ? -1 : 1;
  var latSign = (bytes[0]>>6) & 0x01 ? -1 : 1;
  
  var encLat = ((bytes[0] & 0x3f)<<17)+
               (bytes[1]<<9)+
               (bytes[2]<<1)+
               (bytes[3]>>7);

  var encLon = ((bytes[3] & 0x7f)<<16)+
               (bytes[4]<<8)+
               bytes[5];
  
  var hdop = bytes[8]/10;
  var sats = bytes[9];
  
  const maxHdop = 2;
  const minSats = 5;
  
  if ((hdop < maxHdop) && (sats >= minSats)) {
    // Send only acceptable quality of position to mappers
    payload.latitude = latSign * (encLat * 108 + 53) / 10000000;
    payload.longitude = lonSign * (encLon * 215 + 107) / 10000000;  
    payload.altitude = ((bytes[6]<<8)+bytes[7])-1000;
    payload.accuracy = (hdop*5+5)/10
  } else {
    payload.error = "Need more GPS precision (hdop must be <"+maxHdop+
      " & sats must be >= "+minSats+") current hdop: "+hdop+" & sats:"+sats;
  }
  return payload;
}

```

### Cargo integration

The integration above can be used, create a _Functions_ type _Decoder_ / _Custom Script_ and attach it to a cargo integration. 

- Create a POST on https://cargo.helium.com/api/payloads
- Associate the following decoding function

```
function Decoder(bytes, port) { 
  var decoded = {};
  
  var lonSign = (bytes[0]>>7) & 0x01 ? -1 : 1;
  var latSign = (bytes[0]>>6) & 0x01 ? -1 : 1;
  
  var encLat = ((bytes[0] & 0x3f)<<17)+
               (bytes[1]<<9)+
               (bytes[2]<<1)+
               (bytes[3]>>7);

  var encLon = ((bytes[3] & 0x7f)<<16)+
               (bytes[4]<<8)+
               bytes[5];
  
  var hdop = bytes[8]/10;
  var sats = bytes[9];
  
  const maxHdop = 2;
  const minSats = 5;
  
  if ((hdop < maxHdop) && (sats >= minSats)) {
    // Send only acceptable quality of position to mappers
    decoded.latitude = latSign * (encLat * 108 + 53) / 10000000;
    decoded.longitude = lonSign * (encLon * 215 + 107) / 10000000;  
    decoded.altitude = ((bytes[6]<<8)+bytes[7])-1000;
    decoded.accuracy = (hdop*5+5)/10
    decoded.hdop = hdop;
    decoded.sats = sats;
  } else {
    decoded.error = "Need more GPS precision (hdop must be <"+maxHdop+
      " & sats must be >= "+minSats+") current hdop: "+hdop+" & sats:"+sats;
  }

  return decoded;
}
```


### Creating UF2 files

If you want to create an UF2 file for a drag & drop upload on the WIO Terminal, you can use [uf2 generator](https://seeedjp.github.io/uf2/) online tool. 
* use address 0x4000
* select your binary file compiled from Arduino
* download the file locally


