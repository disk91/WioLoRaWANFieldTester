# Configure cargo

** Work in progress to make this page more User friendly... **

## Cargo integration

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