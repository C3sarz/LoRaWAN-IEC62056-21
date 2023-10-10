# LoRaWAN-IEC62056-21
IEC62056-21 ASCII to LoRaWAN using RS485, powered by RAK11300 (RP2040).

This is a small project I've made to read ISKRA power meters and transmit the specificied OBIS codes through LoRaWAN.

While the code might not be the cleanest nor the most elegant, it is relatively simple and has a decent featureset.
Features:
- Read, parse, and transmit up to 16 OBIS codes.
- Sends values with (decimals removed) as 32bit INTS.
- Supports numbers with up to 3 decimal points.
- Supports downlink configuration.
- Supports saving parameters on nonvolatile flash.
