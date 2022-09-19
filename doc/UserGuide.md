# User Guide

## Seeed bundle
The Seeed bundle is alreeady configured and ready to use. You have no settup to do to start using it. Communications are included up to 200.000 messages.

## Access the data
The Seeed bundle **is already connected** to the different mapping services and Helium cargo. For DiY version, once you connect the device to disk91 backend, you get the link to mappers established. Cargo configuration is documented in the SETUP.md document.

You can access the data (once GPS position is correct), on the following services
- [helium coverage mapping](https://mappers.helium.com)
- [helium tracking - cargo](https://cargo.helium.com)
- [coveragemap mapping](https://coveragemap.net/heatmap/)

## What do I see on the device screen ?

<img src="../img/mainScreen.jpg" alt="LoRaWan tester" width="250"/> <img src="../img/Mainscreen-2.jpg" alt="LoRaWan tester" width="250"/>

You can select the transmission power to be used, the spread-factor (speed), the maximum retries allowed by selecting the parameter to modify with the button located on the top side. Once selected, you change the values with UP & DOWN from the 5 directions button.

When none of these parameters are selected, you can change the mode with the UP & DOWN. The following modes are available:
- Manual : a frame is fired when pushing the 5 ways button. The downlink response is obtained by pooling.
- Auto 5m : a frame is fired automatically every 5 minutes. The downlink response is obtained by pooling.
- Auto 1m : a frame is fired automatically every 1 minute. The downlink response is obtained by pooling.
- Max Rate : a frame is fired as soon as the device can regarding the eventual Duty Cycle. Downlink response will be received later on the flow and will only be monitored from the historical graph.

The status is displayed on the screen and can be:
- Disc - disconnected or not yet connected
- Join - device is joining the network
- Cnx - device has joined, ready to fire messages
- Tx - transmission in progress (orange when doing a retry)
- Dwn - communication in progress to retrieve the downlink containing the network side informations

The Green / Red square on the right is indicating the duty-cycle status when applicable. Green is ready to communicate, Red is duty-cycle with the count-down before being ready.

The Red / Orange / Green circle on the left is indicating the GPS status (when activated). Red is not yet ready, Orange is position acquired but quality is poor to be reported. Green is good quality position ; it will be reported then.

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
<img src="../img/RX RSSI.jpg" alt="Ack Rssi history" width="250"/><img src="../img/RXSNR.jpg" alt="Ack Snr history" width="250"/>

<img src="../img/RETRY.jpg" alt="Uplink Retry history" width="250"/><img src="../img/TXRSSSI.jpg" alt="Network side Rssi history" width="250"/><img src="../img/Hotspots.jpg" alt="Hotspots involved history" width="250"/>

In the historical graph, a red cross is indicating a packet loss a green cross a 0 value.
The TX Rssi graph is displaying a min-max bar, this is why you see just a line for a single hotspot response.

The different screen are from LEFT to RIGHT:
- RX RSSI : Signal level received by the device from the network - it indicates that you are close or far from the LoRaWan gateway responding to the ack
- RX SNR : Signal over Noise reeceived by the devicee from the network - it indicate the link quality from the LoRaWan gateway responding to the ack
- RETRY : number of retry before getting a ack
- TXRSSI : Signal level received by the network from the device - this is indicating the signal quality as perceived by the network on this position. This has a min and max value depending on the number of gateways / hotspot receiving the signal. A strong signal indicate a high communication quality and reduce the risk of message loss.
- HOTSPOTS : Gives the number of hotspots having received the message from the device. It gives the redundancy of the reception. With Seeed bundle, this is limited to 4 hotspots as a maximum at a time.
- DISTANCE : Gives the distance to the different hotspots having received the message from the device. This displays the min and max distance. Distance in km  

### Discovery Mode

The discovery mode send updlink message with no ack to get the maximum of hotspot capturing the information. The backend server is saving these information in a session that can be diplayed at the end of the discovery. On the final step a QR code gives the link to see the discovery results on your smartphone.

To reach the Discovery Mode, push the directional button to the right, up to the last screen.

<img src="../img/Disco_Welcome.jpg" alt="Discovery Welcome" width="250"/>

To start a discovery session, it's better to be already connected to the network, you need to have a GPS coverage and duty-cycle needs to be ended. The Discovery mode duration is over 5 minutes, the progression bar is indicating it's working.

<img src="../img/Disco_Wait.jpg" alt="Discovery Wait for GPS & DC" width="250"/>

Discovery mode do not consider your SF setting, SF10 is always used, the Tx power is considered.

<img src="../img/Disco_Run.jpg" alt="Discovery Running" width="250"/>

At the end, you get the link for displaying the result on your smartphone.

<img src="../img/Disco_qr.jpg" alt="Discovery Qr Code" width="250"/>
<img src="../img/Disco_Result.jpg" alt="Discovery Results" width="250"/>

Please note that the Discovery session are cumulated when performed within the same hour and purged after 2 hours (this is subject to evolve in the future)

