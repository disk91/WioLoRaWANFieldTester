# How to compile and develop WioLoRaWANFieldTester

** Page under construction **

You can use WioLoRaWANFieldTester without a need to compile it by following the [Quick installation guide](SETUP.md). If you want to compile your proper version of it or if you want to contribute on the project by adding feature of fixing some bugs, here are the following steps.

## Required environement

The installation details are available in the related [Wio LoRaWan Field tester on disk91.com](https://www.disk91.com/?p=5187) 

### Required libraries
* GPS - Adafruit GPS library version 1.5.4
* LoRaWAn - Mcci...
* File system - Seeed Arduino FS version 2.0.3 (not yet implemented)
* File system - Seeed Ardunino SFUD version 2.0.1 (not yet implemented)
* Flash - FlashStorage by various version 1.0.0 (not yet implemented)


### Creating UF2 files

If you want to create an UF2 file for a drag & drop upload on the WIO Terminal, you can use [uf2 generator](https://seeedjp.github.io/uf2/) online tool. 
* use address 0x4000
* select your binary file compiled from Arduino
* download the file locally


## Frame format

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

## Decoder for the frame format

Such a decoder can be use if you do not want to use my backend to report the coordinate to the helium mapper

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
    payload.latitude = latSign * (encLat * 108 + 53) / 10000000;
    payload.longitude = lonSign * (encLon * 215 + 107) / 10000000;  
    payload.altitude = ((bytes[6]<<8)+bytes[7])-1000;
    payload.accuracy = (hdop*5+5)/10
    if(payload.accuracy>63) payload.accuracy=63

    decoded.payload = payload;
  } else {
    decoded.error = "Need more GPS precision (hdop must be <"+maxHdop+
      " & sats must be >= "+minSats+") current hdop: "+hdop+" & sats:"+sats;
  }
  return decoded;
}

```
