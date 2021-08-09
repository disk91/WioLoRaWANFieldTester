# Setup your WioLoRaWanFieldTester device

Once you have your WioTerminal and your FieldTester add-on board, you need to proceed the following steps to be ready to use it:

## Install Firmware on the WioTerminal

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

## Setup on Helium console

If you want to use the device with Helium network, you need to declare it in the Helium console.
- Follow the [Helium configuration](ConfigureHelium.md) page details

## Additionnally you can setup Cargo

Cargo is a Tracking platform offered by Helium where you can connect your device and follow it on a map in real time. If you want to configure cargo with the WioLoRaWANFieldTester follow the [Cargo configuration](ConfigureCargo.md) 

## Setup LoRaWan Credentials in the device
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
