# Understand how WioLoRaWANFieldTester works

** Page construction in progress**

The LoRaWan Field tester is basically sending a frame on demand or in regular basis and wait for an ACK. We can obtain the ACK RX power, eventually the number of retries needed to get it. Then this message is passed to a backend service. This service is responding in a downlink with the network reception level (TX rssi) and the number of hotspots involved in the reception.

See more details on how it works on [Wio LoRaWan Field tester on disk91.com](https://www.disk91.com/?p=5187) 
