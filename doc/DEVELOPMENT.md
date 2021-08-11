# How to compile and develop WioLoRaWANFieldTester

You can use WioLoRaWANFieldTester without a need to compile it by following the [Quick installation guide](SETUP.md). If you want to compile your proper version of it or if you want to contribute on the project by adding feature of fixing some bugs, here are the following steps.

## Required environement

The installation details are available in the related [Wio LoRaWan Field tester on disk91.com](https://www.disk91.com/?p=5187) 

### Required software components
* Arduino IDE
* WioTerminal - [Toolsuite](Wio Terminal toolchain)
* GPS - Adafruit GPS library version 1.5.4
* LoRaWAN - MCCI LoRaWAN LMIC library (by IBM, Matthjs Kooljman…) version 3.3.0
* File system - Seeed Arduino FS version 2.0.3 (not yet implemented)
* File system - Seeed Ardunino SFUD version 2.0.1 (not yet implemented)
* Flash - FlashStorage by various version 1.0.0 (not yet implemented)

### Configuring the software components

LoRaWAN library needs to be setup for you local zone: 
  
- Edit the file **lmic_project_config.h** located in _Document/Arduino/libraries/MCCI_LoRaWAN_LMIC_library/project_config_

  You need to uncomment the line corresponding to your zone (us915, eu868...) and comment the others.
  Make sure the other defines are the same as above.
  ```C
  //
  // Select your radio zone and coutry
  //
  //#define CFG_eu868 1
  #define CFG_us915 1
  //#define CFG_au915 1
  ...
  #define CFG_sx1276_radio 1
  //#define LMIC_USE_INTERRUPTS
  #define LMIC_LORAWAN_SPEC_VERSION   LMIC_LORAWAN_SPEC_VERSION_1_0_2
  #define DISABLE_BEACONS
  #define DISABLE_PING
  ```

### Configure the WioLoRaWANFieldTester software

- The **config.h** file contains the configuration defines. Enable / Disable what you need

```C
//#define DEBUG                       // enable extra debug logs on serial
 
#define WITH_SPLASH         1         // Enable splash screen
#define WITH_SPLASH_HELIUM  1         //   display helium logo
#define WITH_SPLASH_TTN     1         //   display TTN logo

#define WITH_GPS                      // if defined the GPS code is enable
#define WITH_LIPO                     // if defined the LiPo status & charging code is enable

```

- The **key.h** file containes the LoRaWAN credential. When all set to 0, the device will expect a serial port configuration as seen in the setup. 

When developping it is more convenient to use a static configuration to avoid reconfiguring the device on every firmware update. For this, you can get the credential as defined in the [Access Helium device credential for developper](ObtainCredsFromHelium.md) section of the documentation.

Then you can directly replace the **key.h** variable content with what you have copy & paste.

```C
#define __DEVEUI { 0x7B, 0xC9, 0xFF, 0x47, 0xE6, 0x49, 0x41, 0xA7 }
#define __APPEUI { 0xD3, 0x77, 0x5B, 0x2C, 0x39, 0x93, 0x58, 0x20 }
#define __APPKEY { 0xE5, 0xC1, 0xD9, 0x44, 0xB4, 0x0D, 0x5D, 0x1C, 0x8B, 0xFB, 0x14, 0x8B, 0x1E, 0x8C, 0x2C, 0xA5 }
```

### Modify and Compile

There is nothing specific about compilation & upload. If you add feature or fixing bug in you private fork, don't forget to propose a pull-request.

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
