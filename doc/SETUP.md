# Setup your WioLoRaWanFieldTester device

Once you have your WioTerminal and your FieldTester add-on board, you need to proceed the following steps to be ready to use it:

## Install Firmware on the WioTerminal

- You can directly upload the following binaries:
  * [EU868 Zone RFM95+GPS](../binaries/WioLoRaWANFieldTester_RFM95_EU868_GPS.uf2)
  * [US915 Zone RFM95+GPS](../binaries/WioLoRaWANFieldTester_RFM95_US915_GPS.uf2)
  * [All Zone LoRa E5 Chassis](../binaries/WioLoRaWANFieldTester_LoRaE5_ALLZONE.uf2)

- Once downloaded
  * Switch the Wio Terminal ON with the lateral button
  * Switch to bootloader mode by pushing down power button twice very quickly. Sequence is ON / RESET / ON / RESET / ON. This mount an Arduino drive on the computer.
  * Drag and drop the selected firmware into that drive.
  * Firmware is flashing, device is rebooting, you should see **"SETUP CREDENTIALS"** on the screen

## Setup on Helium console

If you want to use the device with Helium network, you need to declare it in the Helium console.
- Follow the [Helium configuration](ConfigureHelium.md) page details

## Additionally you can setup Cargo (Optional)

Cargo is a Tracking platform offered by Helium where you can connect your device and follow it on a map in real time. If you want to configure cargo with the WioLoRaWANFieldTester follow the [Cargo configuration](ConfigureCargo.md) 

## Setup LoRaWan Credentials in the device

### Access your device credentials (DevEUI AppEUI and AppKEY)

This operation depends on your network provider:

- For Helium, read the documentation to [obtain them from the console](ObtainCredsFromHelium.md)  

### Configure your device with the credentials (UI way)

On the first boot, the setup screen will be displayed. You can setup the credential and the Zone (when using Wio Terminal chassis LoRa-E5)
 - To navigate into the different items to setup, use the up/down keys
 - To select the right column, use the left/right keys
 - To change the value, use the 1/2/3 button on the top side, they change the value adding respectively 1/2/4 to the current value.
 - To validate and save your configuration, press the 5 direction button.

The device will reboot with the new configuration. 

Once done, if you need to verify or change this setup, restart the device pressing the 5 direction button. After 1 second after rebooting, stop pushing the 5 direction button, you should see the setup screen displayed. 


### Configure your device with the credentials (Serial way)

The device uses the Serial line (Wio Terminal USB) to receives the LoRaWAN configuration. You need to execute a series of commands the WioLoRaWABFieldTester understand to do this setup.

For this, you need to use a serial tool or the command line. The tools I recommand are:
- Windows : [Putty](https://www.putty.org/)
- MacOsX : [Serial](https://www.decisivetactics.com/products/serial/) (7 day free)
- Linux : Minicom 
- Command line, see above

Once you have connected your WioTermial to USB connector of your computer and switch your WioTerminal ON, you may see a new device or com port appearing. Select it in the Serial tool and configure it with a speed of 9600 bits/s, 8 bits, parity None, Stop bit 1.

Now, we will have to pass the different credentials one by one:

* (for Wio Terminal chassis lora E5 only) Set the Zone
```
Z=<Zone> where zone can be EU868, US915, AS923, KR920, IN865, AU915
```
As an example the line to enter in the Serial tool looks like
```
Z=EU868
```
If it works, you should see in the console:
```
ZONE:EU868
OK
```

* Get the DevEUI from the previous step and insert it in the following line:
```
D=<DevEUI>
```
As an example the line to enter in the Serial tool looks like
```
D=DB6B416730000000
```
If it works, you should see in the console:
```
DEVEUI:DB6B416730000000
OK
```
If you see nothing, check your serial settings, make sure WioTerminal is ON. If you see KO only it should be a mistyping, make sure you correctly type the line, please not that the use of Backspace is not authorized and generate errors.

If you see OK, then KO on a second line it can be due to extra end-of-line characters and this is not a problem. OK response indicates it has been correctly taken into account.

* Get the AppEUI from the previous step and insert it in the following line:
```
A=<AppEUI>
```
As an example the line to enter in the Serial tool looks like
```
A=7262F3850E000000
```
If it works, you should see in the console:
```
APPEUI:7262F3850E000000
OK
```

* Get the AppKEY from the previous step and insert it in the following line:
```
K=<DevKEY>
```
As an example the line to enter in the Serial tool looks like
```
K=D2B1F00A451E34931900000000000000
```
If it works, you should see in the console:
```
APPKEY:D2B1F00A451E34931900000000000000
OK
```

* Once the configuration is completed, you should see on the serial tool
```
LoRaWan configuration OK
```

The device now automatically starts to the main dashboard. You are **DONE !**


#### (Alternative) Use the command line

You can use the command line for the setup instead of using a Serial tool. If you proceed the way above, **you do not need to execute this step**.

* On Mac OS
- identify your USB-Serial device (this is the one like above appearing when you plug the WioTerminal on USB)
```
[~] ls /dev/tty.*
/dev/tty.usbmodem123456
/dev/tty. ...
```
- Pass the different command seen in the previous section with the following Serial tool, replacing the tty name by the right value.
```
[~] screen /dev/tty.usbmodem123456 9600
D=DB6B416730000000
...
```
- At the end, quit screen with _CTRL+a k_ touch combination. 







Configuration is ready.
