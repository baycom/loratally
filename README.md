# loratally
A tally system compatible with TSL UMD V5, ATEM, E1.31, MQTT &amp; LoRa

The basic function of this system is providing a relay between an IP based communication and the wireless LoRa standard. Every node in the system can be both: connected to WiFi and/or LoRa. These protocols are supported:

- TSL UMD V5: Standard tally protocol via TCP with support of four different states (off, PGM, PVW, REC), four brightness levels and two lights (left / right)
- ATEM: Tally protocol by Blackmagic Design 
- E1.31: DMX via IP, RGB based
- MQTT: Protocol used in IoT applications


This project is based on ESP32 boards in two different flavours:

- https://heltec.org/project/wifi-lora-32/
- https://www.olimex.com/Products/IoT/ESP32/ESP32-POE-ISO/open-source-hardware
