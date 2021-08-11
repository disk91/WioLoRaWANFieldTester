# Setup your WioLoRaWanFieldTester device

Once you have your WioTerminal and your FieldTester add-on board, you need to proceed the following steps to be ready to use it:

## Install Firmware on the WioTerminal

- You can directly upload the following binaries:
  * [EU868 with GPS enable](binaries/WioLoRaWANFieldTester_EU868_GPS.uf2)
  * [EU868 with GPS disable](binaries/WioLoRaWANFieldTester_EU868_NOGPS.uf2)
  * [US915 with GPS enable](binaries/WioLoRaWANFieldTester_US915_GPS.uf2)
  * [US915 with GPS disable](binaries/WioLoRaWANFieldTester_US915_NOGPS.uf2)
- Once downloaded
  * Switch the Wio Terminal ON with the lateral button
  * Switch to bootloader mode by puching down power button twice very quickly. Sequence is ON / RESET / ON / RESET / ON. This mount an Arduino drive on the computer.
  * Drag and drop the selected firmware into that drive.
  * Fimware is flashing, device is rebooting, you should see **"SETUP CREDENTIALS"** on the screen

## Setup on Helium console

If you want to use the device with Helium network, you need to declare it in the Helium console.
- Follow the [Helium configuration](ConfigureHelium.md) page details

## Additionnally you can setup Cargo (Optional)

Cargo is a Tracking platform offered by Helium where you can connect your device and follow it on a map in real time. If you want to configure cargo with the WioLoRaWANFieldTester follow the [Cargo configuration](ConfigureCargo.md) 

## Setup LoRaWan Credentials in the device

### Access your device credentials (DevEUI AppEUI and AppKEY)

This operation depends on your network provider:

- For Helium, read the documentation to [obtain them from the console](ObtainCredsFromHelium.md)  

### Configure your device with the credentials

The device uses the Serial line (Wioterminal USB) to receives the LoRaWAN configuration. You need to execute a series of commands the WioLoRaWABFieldTester understand to do this setup.

For this, you need to use a serial tool or the command line. The tools I recommand are:
- Windows : [Putty](https://www.putty.org/)
- MacOsX : [Serial](https://www.decisivetactics.com/products/serial/) (7 day free)
- Linux : Minicom 
- Command line, see above

Once you have connected your WioTermial to USB connector of your computer and switch your WioTerminal ON, you may see a new device or com port appearing. Select it in the Serial tool and configure it with a speed of 9600 bits/s, 8 bits, parity None, Stop bit 1.

Now, we will have to pass the different credentials one by one:

* Get the DevEUI from the previous step and insert it in the following line:
```
D=<DevEUI>
```
As an exemple the line to enter in the Serial tool looks like
```
D=DB6B416730000000
```
If it works, you should see in the console:
```
DEVEUI:DB6B416730000000
OK
```
If you see nothing, check your serial settings, make sure WioTerminal is ON. If you see KO only it should be a mistyping, make sure you correctly type the line, please not that the use of Backspace is not authorized and generate errors.

If you see OK, then KO on a second line it can be due to extra end-of-line caracters and this is not a problem. OK response indicates it has been correctly taken into account.

* Get the AppEUI from the prevous step and insert it in the following line:
```
A=<AppEUI>
```
As an exemple the line to enter in the Serial tool looks like
```
A=7262F3850E000000
```
If it works, you should see in the console:
```
APPEUI:7262F3850E000000
OK
```

* Get the AppKEY from the prevous step and insert it in the following line:
```
K=<DevKEY>
```
As an exemple the line to enter in the Serial tool looks like
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
- identify your USB-Serial device (this is the one like above apearing when you plug the WioTerminal on USB)
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







onfiguration and is ready.
