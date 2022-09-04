# loratally
A tally system compatible with TSL UMD V5, ATEM, E1.31, MQTT &amp; LoRa

The basic function principle of this system is providing a bidirectional relay between an IP based communication and the wireless LoRa standard. Every node in the system can be both: connected to WiFi and/or LoRa. These protocols are supported:

- TSL UMD V5: Standard tally protocol via TCP with support of four different states (off, PGM, PVW, REC), four brightness levels and two lights (left / right)
- ATEM: Tally protocol by Blackmagic Design 
- E1.31: DMX via IP, RGB based
- MQTT: Protocol used in IoT applications
- LoRa: Proprietary protocol for up to 64 tally lights

The function principle is as follows:

- TSL UMD V5: If a TSL port is configured tally states (off, PGM, PVW, REC) and brightness of up to 32 channels with different RH / LH light states are forwareded via LoRa. Text messages are only displayed on the ip connected system.
- ATEM: If a ATEM host is configured tally states (off, PGW, PVW) of up to 32 channels received via network are forwarded via LoRa.
- E1.31: Each tally light is addressed via its DMX channel. Every 3 channels make one tally: 
  - ch1: red tally 1, ch2: green tally 1, ch3: blue tally 1 
  - ch4: red tally 2, ch5: green tally 2, ch6: blue tally 2
  - ...
- E1.31: Each tally can be adressed separately via the DMX universe field:
  - universe 1: ch1: red tally 1, ch2: green tally 1, ch3: blue tally 1
  - universe 2: ch1: red tally 2, ch2: green tally 2, ch3: blue tally 2
  - ...
- MQTT: If a MQTT host is configured each system subscribes to a path like tally/control/<tally id> and waits for a JSON object with these values:
  ```
  {
    "addr": 1,
    "state": 2,
    "brightness": 3
  } 
  ```
  alternatively you can set a light via RGB values:
  ```
  {
    "addr": 1,
    "r": 255,
    "g": 0,
    "b": 0
  } 
  ```

This project is based on ESP32 boards in two different flavours:

- https://heltec.org/project/wifi-lora-32/ this board is intended as a tally light primarily connected via LoRa. Any number of WS2812 (NeoPixel) LEDs might be connected to PIN23 (PixelPin defined in main.h). Usually you connect two of them where the first one is RHS and the second one is LHS.
- https://www.olimex.com/Products/IoT/ESP32/ESP32-POE-ISO/open-source-hardware this board is intended to be used as a gateway to ethernet with PoE.

