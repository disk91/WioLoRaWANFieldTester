## Release 1.9
- Fix US915 / SF10 
- Prevent bundle users to modify the credentials
- Prevent downlink to be infinite loop

## Release 1.8
- Support TTN
- Improve Downlink reception and frame counter
- Fix trouble when displaying SF & Power for the first time
- Add the discovery mode

## Release 1.7
- Fix a bug that sometime infinite loop the device after a couple of hours

## Release 1.6
- Add support for GPS L76K used in certain version of LoRa chassis (E5)

## Release 1.5
- Fix RFM95 compilation & misc fix includes new binaries
- Includes the GPS serial speed fix for some LoRa-E5 board wrongly configured
- Reduce communication period after 30 minute non moving device

## Release 1.4
- Battery status now empty when Wio Battery chassis non connected
- Non duty cycle zone now have 25s Duty Cycle instead of 5s ( preserve backend capacities )
- Display the wio terminal name on boot (based on DEVEUI)
- Reduce communication period when the device is out of GPS coverage or not moving
- fix the bar display with last value displayed on first column

## Release 1.3
- initialization of the change log